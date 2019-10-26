/**
  ******************************************************************************
  * @file    subscribe_publish_sensor_values.c
  * @author  MCD Application Team
  * @brief   Control of the measurement sampling and MQTT reporting loop.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#ifdef CLD_OTA
#include "rfu.h"
#endif
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_jobs_interface.h"
#include "iot_flash_config.h"
#ifdef SENSOR
#include "sensors_data.h"
#endif
#include "msg.h"

/* Private defines ------------------------------------------------------------*/
#define MQTT_CONNECT_MAX_ATTEMPT_COUNT  3
#define TELEMETRY_INTERVAL              10    /*< When the telemetry is active, the data are published every TELEMETRY_INTERVAL seconds. */
#define TELEMETRY_LIFETIME              300   /*< The telemetry publication stops after TELEMETRY_LIFETIME seconds. */

#define USE_JOBS

#define aws_json_pre        "{\"state\":{\"reported\":"
#define aws_json_desired    "{\"state\":{\"desired\":"
#define aws_json_post       "}}"

/* Private variables ---------------------------------------------------------*/
static const char *pDeviceName;
static uint8_t sim_bp_pushed = BP_NOT_PUSHED;
static bool ledstateOn = false;
static char cPTopicName_data[MAX_SHADOW_TOPIC_LENGTH_BYTES] = ""; /* Publish Topic */
static char cPTopicName[MAX_SHADOW_TOPIC_LENGTH_BYTES] = ""; /* Publish Topic */
static char cSTopicName[MAX_SHADOW_TOPIC_LENGTH_BYTES] = ""; /* Subscribe Topic */

#ifdef USE_JOBS
static char topicToSubscribeGetPending[MAX_JOB_TOPIC_LENGTH_BYTES] = "";
static char topicToSubscribeNotifyNext[MAX_JOB_TOPIC_LENGTH_BYTES] = "";
static char topicToSubscribeGetNext[MAX_JOB_TOPIC_LENGTH_BYTES] = "";
static char topicToSubscribeUpdateAccepted[MAX_JOB_TOPIC_LENGTH_BYTES] = "";
static char topicToSubscribeUpdateRejected[MAX_JOB_TOPIC_LENGTH_BYTES] = "";
static char topicToPublishGetPending[MAX_JOB_TOPIC_LENGTH_BYTES] = "";
static char topicToPublishGetNext[MAX_JOB_TOPIC_LENGTH_BYTES] = "";
static jsmn_parser jsonParser;
static jsmntok_t jsonTokenStruct[MAX_JSON_TOKEN_EXPECTED];
static int32_t tokenCount;
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
bool g_continue = true;     /*< Controls the MQTT client loop. */
#ifdef CLD_OTA
static bool g_ExecuteFOTA;
#define FOTA_URI_LEN 300
char g_firmware_update_uri[FOTA_URI_LEN+1];
iot_state_t write_ota_state;
#define FOTA_INSTALLATION_NOT_REQUESTED ((uint8_t)0)
#define FOTA_INSTALLATION_REQUESTED ((uint8_t)1)
#endif

/* Private function prototypes -----------------------------------------------*/
static void MQTTcallbackHandler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
int32_t comp_left_ms(uint32_t init, uint32_t now, uint32_t timeout);

#ifdef USE_JOBS
static void iot_get_pending_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
static void iot_next_job_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,  IoT_Publish_Message_Params *params, void *pData);
static void iot_update_accepted_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
static void iot_update_rejected_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
#endif /* USE_JOBS */

/* Functions Definition ------------------------------------------------------*/
int cloud_device_enter_credentials(void)
{
  int ret = 0;
  iot_config_t iot_config;

  memset(&iot_config, 0, sizeof(iot_config_t));

  printf("\nEnter server address: (example: xxx.iot.region.amazonaws.com) \n");
  getInputString(iot_config.server_name, USER_CONF_SERVER_NAME_LENGTH);
  msg_info("read: --->\n%s\n<---\n", iot_config.server_name);

  printf("\nEnter device name: (example: mything1) \n");
  getInputString(iot_config.device_name, USER_CONF_DEVICE_NAME_LENGTH);
  msg_info("read: --->\n%s\n<---\n", iot_config.device_name);

  if (setIoTDeviceConfig(&iot_config) != 0)
  {
    ret = -1;
    msg_error("Failed programming the IoT device configuration to Flash.\n");
  }

  return ret;
}


bool app_needs_root_ca(void)
{
  return true;
}


bool app_needs_device_keypair(void)
{
  return true;
}


bool app_needs_iot_config(void)
{
  return true;
}


/**
* @brief MQTT disconnect callback hander
*
* @param pClient: pointer to the AWS client structure
* @param data:
* @return no return
*/
static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
  msg_warning("MQTT Disconnect\n");
  IoT_Error_t rc = FAILURE;

  if (NULL == data)
  {
    return;
  }

  AWS_IoT_Client *client = (AWS_IoT_Client *)data;

  if (aws_iot_is_autoreconnect_enabled(client))
  {
    msg_info("Auto Reconnect is enabled, Reconnecting attempt will start now\n");
  }
  else
  {
    msg_warning("Auto Reconnect not enabled. Starting manual reconnect...\n");
    rc = aws_iot_mqtt_attempt_reconnect(client);

    if (NETWORK_RECONNECTED == rc)
    {
      msg_warning("Manual Reconnect Successful\n");
    }
    else
    {
      msg_warning("Manual Reconnect Failed - %d\n", rc);
    }
  }
}

/* Exported functions --------------------------------------------------------*/

/**
* @brief MQTT subscriber callback hander
*
* called when data is received from AWS IoT Thing (message broker)
* @param MQTTCallbackParams type parameter
* @return no return
*/
static void MQTTcallbackHandler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData)
{
  const char msg_on[]  = "{\"state\":{\"reported\":{\"LED_value\":\"On\"}}}";
  const char msg_off[] = "{\"state\":{\"reported\":{\"LED_value\":\"Off\"}}}";
  const char *msg = NULL;
  char buf[4];
  jsmntok_t *state;
#ifndef  AWS_IOT_DASHBOARD
  jsmntok_t *desired;
#endif
  jsmntok_t *value;
  IoT_Publish_Message_Params sendParams =
  {
    .qos = QOS1,
    .isRetained = 0,
    .isDup = 0,
    .id = 0,
    .payload = NULL,
    .payloadLen = 0
  };

  msg_info("\nMQTT subscribe callback......\n");
  msg_info("%.*s\n", (int)params->payloadLen, (char *)params->payload);

  /* If a new desired LED state is received, change the LED state. */
  /* Parse the received message. */
  jsmn_init(&jsonParser);
  tokenCount = jsmn_parse(&jsonParser, params->payload, (int) params->payloadLen, jsonTokenStruct, MAX_JSON_TOKEN_EXPECTED);

  if (tokenCount < 0)
  {
    IOT_WARN("Failed to parse JSON: %d", tokenCount);
    return;
  }

  /* Assume the top-level element is an object */
  if (tokenCount < 1 || jsonTokenStruct[0].type != JSMN_OBJECT)
  {
    IOT_WARN("Top Level is not an object");
    return;
  }

  state = findToken("state", params->payload, jsonTokenStruct);

  if (state)
  {
#ifndef  AWS_IOT_DASHBOARD
    desired = findToken("desired", params->payload, state);
    if (desired)
#endif
    {
#ifdef  AWS_IOT_DASHBOARD
      value = findToken("LED_value", params->payload, state);
#else
      value = findToken("LED_value", params->payload, desired);
#endif
      if (value)
      {
        if (parseStringValue(buf, sizeof(buf), params->payload, value) == AWS_SUCCESS)
        {
          if (strcmp(buf, "On") == 0)
          {
            ledstateOn = true;
            msg = msg_on;
          }
          else if (strcmp(buf, "Off") == 0)
          {
            ledstateOn = false;
            msg = msg_off;
          }
          else
          {
            msg_error("Illegal LED_value desired value %s\n", buf);
          }
        }
        else
        {
          msg_error("Could not parse the LED_value string.\n");
        }
      }
    }
  }

  /* Set and report the new LED state to the MQTT broker. */
  if (msg != NULL)
  {
    msg_info("LED %s!\n", buf);
    Led_SetState(ledstateOn);

    sendParams.payload = (void *) msg;
    sendParams.payloadLen = strlen(msg) + 1;
    IoT_Error_t rc = aws_iot_mqtt_publish(pClient, cPTopicName, strlen(cPTopicName), &sendParams);

    if (rc == AWS_SUCCESS)
    {
      msg_info("\nPublished the new LED status to topic %s:", cPTopicName);

      msg_info("%s\n", msg);
    }
  }
}

#ifdef USE_JOBS
static void iot_get_pending_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                             IoT_Publish_Message_Params *params, void *pData)
{
  IOT_UNUSED(pData);
  IOT_UNUSED(pClient);
  IOT_INFO("\nJOB_GET_PENDING_TOPIC callback");
  IOT_INFO("topic: %.*s", topicNameLen, topicName);
  IOT_INFO("payload: %.*s", (int) params->payloadLen, (char *)params->payload);

  jsmn_init(&jsonParser);

  tokenCount = jsmn_parse(&jsonParser, params->payload, (int) params->payloadLen, jsonTokenStruct, MAX_JSON_TOKEN_EXPECTED);

  if (tokenCount < 0)
  {
    IOT_WARN("Failed to parse JSON: %d", tokenCount);
    return;
  }

  /* Assume the top-level element is an object */
  if (tokenCount < 1 || jsonTokenStruct[0].type != JSMN_OBJECT)
  {
    IOT_WARN("Top Level is not an object");
    return;
  }

  jsmntok_t *jobs;

  jobs = findToken("inProgressJobs", params->payload, jsonTokenStruct);

  if (jobs)
  {
    IOT_INFO("inProgressJobs: %.*s", jobs->end - jobs->start, (char *)params->payload + jobs->start);
  }

  jobs = findToken("queuedJobs", params->payload, jsonTokenStruct);

  if (jobs)
  {
    IOT_INFO("queuedJobs: %.*s", jobs->end - jobs->start, (char *)params->payload + jobs->start);
  }
}

static void iot_next_job_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                          IoT_Publish_Message_Params *params, void *pData)
{
  char topicToPublishUpdate[MAX_JOB_TOPIC_LENGTH_BYTES];
  char messageBuffer[200];

  IOT_UNUSED(pData);
  IOT_UNUSED(pClient);
  IOT_INFO("\nJOB_NOTIFY_NEXT_TOPIC / JOB_DESCRIBE_TOPIC($next) callback");
  IOT_INFO("topic: %.*s", topicNameLen, topicName);
  IOT_INFO("payload: %.*s", (int) params->payloadLen, (char *)params->payload);

  jsmn_init(&jsonParser);

  tokenCount = jsmn_parse(&jsonParser, params->payload, (int) params->payloadLen, jsonTokenStruct, MAX_JSON_TOKEN_EXPECTED);

  if (tokenCount < 0)
  {
    IOT_WARN("Failed to parse JSON: %d", tokenCount);
    return;
  }

  /* Assume the top-level element is an object */
  if (tokenCount < 1 || jsonTokenStruct[0].type != JSMN_OBJECT)
  {
    IOT_WARN("Top Level is not an object");
    return;
  }

  jsmntok_t *tokExecution;

  tokExecution = findToken("execution", params->payload, jsonTokenStruct);

  if (tokExecution)
  {
    IOT_INFO("execution: %.*s", tokExecution->end - tokExecution->start, (char *)params->payload + tokExecution->start);

    jsmntok_t *tok;

    tok = findToken("jobId", params->payload, tokExecution);

    if (tok)
    {
      IoT_Error_t rc;
      char jobId[MAX_SIZE_OF_JOB_ID + 1];
#define MAX_SIZE_OF_STATUS 20
      char jobStatus[MAX_SIZE_OF_STATUS + 1];
      AwsIotJobExecutionUpdateRequest updateRequest;

      rc = parseStringValue(jobId, MAX_SIZE_OF_JOB_ID + 1, params->payload, tok);
      if (AWS_SUCCESS != rc)
      {
        IOT_ERROR("parseStringValue returned error : %d ", rc);
        return;
      }

      IOT_INFO("jobId: %s", jobId);

      tok = findToken("status", params->payload, tokExecution);

      rc = parseStringValue(jobStatus, MAX_SIZE_OF_STATUS + 1, params->payload, tok);
      if (AWS_SUCCESS != rc)
      {
        IOT_ERROR("parseStringValue returned error : %d ", rc);
        return;
      }
      IOT_INFO("jobStatus: %s\n", jobStatus);

      tok = findToken("jobDocument", params->payload, tokExecution);

      /*
       * Do your job processing here.
       */

      if (tok)
      {
        jsmntok_t *tokmyOperation;
        IOT_INFO("jobDocument: %.*s", tok->end - tok->start, (char *)params->payload + tok->start);

        tokmyOperation = findToken("myOperation", params->payload, tok);
        if (tokmyOperation)
        {
          IOT_INFO("myOperation: %.*s", tokmyOperation->end - tokmyOperation->start, (char *)params->payload + tokmyOperation->start);
          const char single_push[] = "simulate_single_push";
          const char multiple_push[] = "simulate_multiple_push";
          if (strncmp(single_push, (char *)params->payload + tokmyOperation->start, strlen(single_push)) == 0)
          {
            sim_bp_pushed = BP_SINGLE_PUSH;
            updateRequest.status = JOB_EXECUTION_SUCCEEDED;
            updateRequest.statusDetails = "{\"statusDetails\":\"Success\"}";
          }
          else if (strncmp(multiple_push, (char *)params->payload + tokmyOperation->start, strlen(multiple_push)) == 0)
          {
            sim_bp_pushed = BP_MULTIPLE_PUSH;
            updateRequest.status = JOB_EXECUTION_SUCCEEDED;
            updateRequest.statusDetails = "{\"statusDetails\":\"Success\"}";
          }
          else
          {
            msg_error("Unknown job myOperation type.\n");
            updateRequest.status = JOB_EXECUTION_FAILED;
            updateRequest.statusDetails = "{\"statusDetails\":\"Unknown job myOperation type.\"}";
          }
        }
        else
        {
#ifdef CLD_OTA
          jsmntok_t *fwUpdateToken = NULL;

          fwUpdateToken = findToken("firmwareUpdate", params->payload, tok);
          if (fwUpdateToken)
          {
            const iot_state_t * read_ota_state = NULL;

            getIoTState(&read_ota_state);
            msg_info("firmwareUpdate: %.*s\n", fwUpdateToken->end - fwUpdateToken->start,
                     (char *)params->payload + fwUpdateToken->start);
            if (read_ota_state->fota_state == FOTA_INSTALLATION_REQUESTED)
            {
              /* firmware update was already started */
              char current_fw_version[16];
              int ret = 0;

              snprintf(current_fw_version, sizeof(current_fw_version), "%d.%d.%d", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);

              msg_info("A FOTA installation was requested previously.\n");
              msg_info("prev_fw_version: %s\n", read_ota_state->prev_fw_version);
              msg_info("current_fw_version: %s\n", current_fw_version);
              if (strcmp(read_ota_state->prev_fw_version, current_fw_version) != 0)
              {
                msg_info("JOB_EXECUTION_SUCCEEDED\n");
                updateRequest.status = JOB_EXECUTION_SUCCEEDED;
                updateRequest.statusDetails = "{\"statusDetails\":\"SUCCEEDED\"}";
              }
              else
              {
                /* if the versions are the same, we consider the process is failed */
                msg_info("JOB_EXECUTION_FAILED\n");
                updateRequest.status = JOB_EXECUTION_FAILED;
                updateRequest.statusDetails = "{\"statusDetails\":\"FAILED\"}";
              }
              /* Reset the Flash information until next FOTA request */
              memset(&write_ota_state, 0x00, sizeof(iot_state_t));

              write_ota_state.fota_state = FOTA_INSTALLATION_NOT_REQUESTED;
              /* No need to update write_ota_state.prev_fw_version: already filled with 0s */

              ret = setIoTState(&write_ota_state);

              if (0 != ret)
              {
                msg_error("setIoTDeviceConfig(FOTA_INSTALLATION_NOT_REQUESTED) failed.\n");
              } /* else nothing to do */
            }
            else
            {
              /* Firmware update was not already started - start it */
              msg_info("No FOTA installation was recorded previously.\nExiting from main loop to start Firmware download.\n");
              g_ExecuteFOTA = true;
              strncpy(g_firmware_update_uri, (char *)params->payload + fwUpdateToken->start, fwUpdateToken->end - fwUpdateToken->start);
              g_continue = false; /* exit from MQTT loop to do the firmware update */
              updateRequest.status = JOB_EXECUTION_IN_PROGRESS;
              updateRequest.statusDetails = "{\"statusDetails\":\"10\"}";
            }
          }
          else
#endif
          {
            updateRequest.status = JOB_EXECUTION_REJECTED;
            updateRequest.statusDetails = "{\"statusDetails\":\"unknown job\"}";
          }
        }
      }
      else
      {
        updateRequest.status = JOB_EXECUTION_FAILED;
        updateRequest.statusDetails = "{\"failureDetail\":\"Unable to process job document\"}";
      }

      updateRequest.expectedVersion = 0;
      updateRequest.executionNumber = 0;
      updateRequest.includeJobExecutionState = false;
      updateRequest.includeJobDocument = false;
      updateRequest.clientToken = NULL;

      rc = aws_iot_jobs_send_update(pClient, QOS0, pDeviceName, jobId, &updateRequest,
                                    topicToPublishUpdate, sizeof(topicToPublishUpdate), messageBuffer, sizeof(messageBuffer));
    }
  }
  else
  {
    IOT_INFO("execution property not found, nothing to do");
  }
}

static void iot_update_accepted_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                                 IoT_Publish_Message_Params *params, void *pData)
{
  IOT_UNUSED(pData);
  IOT_UNUSED(pClient);
  IOT_INFO("\nJOB_UPDATE_TOPIC / accepted callback");
  IOT_INFO("topic: %.*s", topicNameLen, topicName);
  IOT_INFO("payload: %.*s", (int) params->payloadLen, (char *)params->payload);
}

static void iot_update_rejected_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                                 IoT_Publish_Message_Params *params, void *pData)
{
  IOT_UNUSED(pData);
  IOT_UNUSED(pClient);
  IOT_INFO("\nJOB_UPDATE_TOPIC / rejected callback");
  IOT_INFO("topic: %.*s", topicNameLen, topicName);
  IOT_INFO("payload: %.*s", (int) params->payloadLen, (char *)params->payload);

  /* Do error handling here for when the update was rejected */
}
#endif /* USE_JOBS */

/**
* @brief main entry function to AWS IoT code
*
* @param no parameter
* @return AWS_SUCCESS: 0
          FAILURE: -1
*/
void cloud_run(void const *arg)
{
  const char *serverAddress = NULL;
  const char *pCaCert;
  const char *pClientCert;
  const char *pClientPrivateKey;
  char* cPayload = NULL;
  char const *deviceName;
  int connectCounter;
  IoT_Error_t rc = FAILURE;
#ifdef CLD_OTA
  int ret = 0;
#endif
#ifdef SENSOR
  uint32_t start_telemetry_time_ms = HAL_GetTick();
  uint32_t last_telemetry_time_ms = start_telemetry_time_ms;
  int32_t left_before_publish_ms;
  int32_t left_before_lifetime_end_ms;
#endif
  uint8_t bp_pushed;

  AWS_IoT_Client client;
  memset(&client, 0, sizeof(AWS_IoT_Client));
  IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
  IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

#ifdef CLD_OTA
  g_ExecuteFOTA = false;
#endif
  getIoTDeviceConfig(&deviceName);
  if (strlen(deviceName) >= MAX_SIZE_OF_THING_NAME)
  {
    msg_error("The length of the device name stored in the iot user configuration is larger than the AWS client MAX_SIZE_OF_THING_NAME.\n");
    return;
  }

#ifdef  AWS_IOT_DASHBOARD
  snprintf(cPTopicName_data, sizeof(cPTopicName_data), "telemetrydata/sensors" );
  snprintf(cSTopicName, sizeof(cSTopicName), AWS_DEVICE_SHADOW_PRE "%s" AWS_DEVICE_SHADOW_UPDATE_DELTA_TOPIC, deviceName);
  snprintf(cPTopicName, sizeof(cPTopicName), AWS_DEVICE_SHADOW_PRE "%s" AWS_DEVICE_SHADOW_UPDATE_TOPIC, deviceName);
#else
  snprintf(cPTopicName_data, sizeof(cPTopicName_data), AWS_DEVICE_SHADOW_PRE "%s" AWS_DEVICE_SHADOW_UPDATE_TOPIC, deviceName);
  snprintf(cPTopicName, sizeof(cPTopicName), AWS_DEVICE_SHADOW_PRE "%s" AWS_DEVICE_SHADOW_UPDATE_TOPIC, deviceName);
  snprintf(cSTopicName, sizeof(cSTopicName), AWS_DEVICE_SHADOW_PRE "%s" AWS_DEVICE_SHADOW_UPDATE_ACCEPTED_TOPIC, deviceName);
#endif

  msg_info("AWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

  getServerAddress(&serverAddress);
  getTLSKeys(&pCaCert, &pClientCert, &pClientPrivateKey);
  mqttInitParams.enableAutoReconnect = false; /* We enable this later below */
  mqttInitParams.pHostURL = (char *) serverAddress;
  mqttInitParams.port = AWS_IOT_MQTT_PORT;
  mqttInitParams.pRootCALocation = (char *) pCaCert;
  mqttInitParams.pDeviceCertLocation = (char *) pClientCert;
  mqttInitParams.pDevicePrivateKeyLocation = (char *) pClientPrivateKey;
  mqttInitParams.mqttCommandTimeout_ms = 20000;
  mqttInitParams.tlsHandshakeTimeout_ms = 5000;
  mqttInitParams.isSSLHostnameVerify = true;
  mqttInitParams.disconnectHandler = disconnectCallbackHandler;
  mqttInitParams.disconnectHandlerData = NULL;

  rc = aws_iot_mqtt_init(&client, &mqttInitParams);

  if (AWS_SUCCESS != rc)
  {
    msg_error("aws_iot_mqtt_init returned error : %d\n", rc);
    return;
  }

  getIoTDeviceConfig(&pDeviceName);
  connectParams.keepAliveIntervalInSec = 30;
  connectParams.isCleanSession = true;
  connectParams.MQTTVersion = MQTT_3_1_1;
  connectParams.pClientID = (char *) pDeviceName;
  connectParams.clientIDLen = (uint16_t) strlen(pDeviceName);
  connectParams.isWillMsgPresent = false;


  connectCounter = 0;

  do
  {
    connectCounter++;
    printf("MQTT connection in progress:   Attempt %d/%d ...\n", connectCounter, MQTT_CONNECT_MAX_ATTEMPT_COUNT);
    rc = aws_iot_mqtt_connect(&client, &connectParams);
  }
  while ((rc != AWS_SUCCESS) && (connectCounter < MQTT_CONNECT_MAX_ATTEMPT_COUNT));

  if (AWS_SUCCESS != rc)
  {
    msg_error("\nError(%d) connecting to %s:%d\n", rc, mqttInitParams.pHostURL, mqttInitParams.port);
    return;
  }
  else
  {
    printf("\nConnected to %s:%d\n", mqttInitParams.pHostURL, mqttInitParams.port);
  }

  /*
  * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
  *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
  *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
  */
  rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);

  if (AWS_SUCCESS != rc)
  {
    msg_error("Unable to set Auto Reconnect to true - %d\n", rc);

    if (aws_iot_mqtt_is_client_connected(&client))
    {
      aws_iot_mqtt_disconnect(&client);
    }

    return;
  }

#ifdef USE_JOBS
  /* Subscribe to jobs topics */
  rc = aws_iot_jobs_subscribe_to_job_messages(
         &client, QOS0, pDeviceName, NULL, JOB_GET_PENDING_TOPIC, JOB_WILDCARD_REPLY_TYPE,
         iot_get_pending_callback_handler, NULL, topicToSubscribeGetPending, sizeof(topicToSubscribeGetPending));

  if (AWS_SUCCESS != rc)
  {
    msg_error("Error subscribing JOB_GET_PENDING_TOPIC: %d\n", rc);
    return;
  }

  rc = aws_iot_jobs_subscribe_to_job_messages(
         &client, QOS0, pDeviceName, NULL, JOB_NOTIFY_NEXT_TOPIC, JOB_REQUEST_TYPE,
         iot_next_job_callback_handler, NULL, topicToSubscribeNotifyNext, sizeof(topicToSubscribeNotifyNext));

  if (AWS_SUCCESS != rc)
  {
    msg_error("Error subscribing JOB_NOTIFY_NEXT_TOPIC: %d\n", rc);
    return;
  }

  rc = aws_iot_jobs_subscribe_to_job_messages(
         &client, QOS0, pDeviceName, JOB_ID_NEXT, JOB_DESCRIBE_TOPIC, JOB_WILDCARD_REPLY_TYPE,
         iot_next_job_callback_handler, NULL, topicToSubscribeGetNext, sizeof(topicToSubscribeGetNext));

  if (AWS_SUCCESS != rc)
  {
    msg_error("Error subscribing JOB_DESCRIBE_TOPIC ($next): %d\n", rc);
    return;
  }

  rc = aws_iot_jobs_subscribe_to_job_messages(
         &client, QOS0, pDeviceName, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_ACCEPTED_REPLY_TYPE,
         iot_update_accepted_callback_handler, NULL, topicToSubscribeUpdateAccepted, sizeof(topicToSubscribeUpdateAccepted));

  if (AWS_SUCCESS != rc)
  {
    msg_error("Error subscribing JOB_UPDATE_TOPIC/accepted: %d\n", rc);
    return;
  }

  rc = aws_iot_jobs_subscribe_to_job_messages(
         &client, QOS0, pDeviceName, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_REJECTED_REPLY_TYPE,
         iot_update_rejected_callback_handler, NULL, topicToSubscribeUpdateRejected, sizeof(topicToSubscribeUpdateRejected));

  if (AWS_SUCCESS != rc)
  {
    msg_error("Error subscribing JOB_UPDATE_TOPIC/rejected: %d\n", rc);
    return;
  }

  /* Get the list of the pending jobs */
  rc = aws_iot_jobs_send_query(&client, QOS0, pDeviceName, NULL, NULL, topicToPublishGetPending, sizeof(topicToPublishGetPending), NULL, 0, JOB_GET_PENDING_TOPIC);

  AwsIotDescribeJobExecutionRequest describeRequest;
  describeRequest.executionNumber = 0;
  describeRequest.includeJobDocument = true;
  describeRequest.clientToken = NULL;

  rc = aws_iot_jobs_describe(&client, QOS0, pDeviceName, JOB_ID_NEXT, &describeRequest, topicToPublishGetNext, sizeof(topicToPublishGetNext), NULL, 0);
#endif // USE_JOBS

  /* Subscribe to the "shadow accepted" topic */
  rc = aws_iot_mqtt_subscribe(&client, cSTopicName, strlen(cSTopicName), QOS0, MQTTcallbackHandler, NULL);

  if (AWS_SUCCESS != rc)
  {
    msg_error("Error subscribing : %d\n", rc);
    return;
  }
  else
  {
    msg_info("Subscribed to topic %s\n", cSTopicName);
  }

  IoT_Publish_Message_Params paramsQOS1 =
  {
    .qos = QOS1,
    .isRetained = 0,
    .isDup = 0,
    .id = 0,
    .payload = NULL,
    .payloadLen = 0
  };

  printf("Press the User button (Blue) to publish the LED desired value on the %s topic\n", cPTopicName);

  while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || AWS_SUCCESS == rc)
         && g_continue)
  {
    /* Max time the yield function will wait for read messages */
    rc = aws_iot_mqtt_yield(&client, 10);

    if (NETWORK_ATTEMPTING_RECONNECT == rc)
    {
      /* Delay to let the client reconnect */
      HAL_Delay(1000);
      msg_info("Attempting to reconnect\n");
      /* If the client is attempting to reconnect we will skip the rest of the loop */
      continue;
    }
    if (NETWORK_RECONNECTED == rc)
    {
      msg_info("Reconnected.\n");
    }

    /* STEP: User interaction */
    bp_pushed = (sim_bp_pushed != BP_NOT_PUSHED) ? sim_bp_pushed : Button_WaitForMultiPush(500);
    sim_bp_pushed = BP_NOT_PUSHED;

    /* exit loop on long push  */
    if (bp_pushed == BP_MULTIPLE_PUSH)
    {
      msg_info("\nPushed button perceived as a *double push*. Terminates the application.\n");
      break;
    }

    /* create desired message */
    if (!cPayload)
    {
      cPayload = malloc(AWS_IOT_MQTT_TX_BUF_LEN);
      if (!cPayload)
      {
        msg_error("Unable to allocate memory for the Payload\n");
      }
    }

    if (bp_pushed == BP_SINGLE_PUSH)
    {
      printf("Sending the desired LED state to AWS.\n");
      ledstateOn = !ledstateOn;


      (void) snprintf(cPayload, AWS_IOT_MQTT_TX_BUF_LEN, "%s{\"LED_value\":\"%s\"}%s",
                      aws_json_desired, (ledstateOn) ? "On" : "Off", aws_json_post);

      paramsQOS1.payload = cPayload;
      paramsQOS1.payloadLen = strlen(cPayload) + 1;

      do
      {
        rc = aws_iot_mqtt_publish(&client, cPTopicName, strlen(cPTopicName), &paramsQOS1);

        if (rc == AWS_SUCCESS)
        {
          printf("\nPublished to topic %s:", cPTopicName);
          printf("%s\n", cPayload);
        }
      } while (MQTT_REQUEST_TIMEOUT_ERROR == rc);
    }

#ifdef  SENSOR
    left_before_publish_ms = comp_left_ms(last_telemetry_time_ms, HAL_GetTick(), TELEMETRY_INTERVAL * 1000);
    left_before_lifetime_end_ms = comp_left_ms(start_telemetry_time_ms, HAL_GetTick(), TELEMETRY_LIFETIME * 1000);

    if (left_before_publish_ms <= 0)
    {
      last_telemetry_time_ms = HAL_GetTick();

      if (left_before_lifetime_end_ms > 0)
      {
#ifdef  AWS_IOT_DASHBOARD
        (void) PrepareSensorsData(cPayload, AWS_IOT_MQTT_TX_BUF_LEN, (char *)(deviceName) );
#else
        (void) PrepareSensorsData(cPayload, AWS_IOT_MQTT_TX_BUF_LEN, NULL );
#endif
        paramsQOS1.payload = cPayload;
        paramsQOS1.payloadLen = strlen(cPayload) + 1;

        do
        {
          rc = aws_iot_mqtt_publish(&client, cPTopicName_data, strlen(cPTopicName_data), &paramsQOS1);

          if (rc == AWS_SUCCESS)
          {
            printf("\nPublished to topic %s:\n", cPTopicName_data);
            printf("%s\n", cPayload);
          }
        }
        while ((MQTT_REQUEST_TIMEOUT_ERROR == rc));
      }
      else
      {
        printf("End of telemetry lifetime reached: Publication skipped.\n");
      }
    }
#endif

  } /* End of while */

  msg_debug("Exit from subscribe_publish_sensor_values() main loop.\n");
  /* Wait for all the messages to be received */
  aws_iot_mqtt_yield(&client, 10);

  rc = aws_iot_mqtt_disconnect(&client);
  /* Free up payload table */
  if (cPayload)
  {
    free(cPayload);
  }

#ifdef CLD_OTA
  if (g_ExecuteFOTA)
  {
    IOT_INFO("Updating Firmware with URL: %s\n", g_firmware_update_uri);
    ret = rfu_update(g_firmware_update_uri, lUserConfigPtr->tls_root_ca_cert);

    if (ret == RFU_OK)
    {
      IOT_INFO("  -- Image downloaded.\n");

      /*
       * Memorize that the bootloader will run the Firmware Installation procedure.
       * This will be useful at next boot to determine the proper status ('Error' or 'Current') to be reported.
       */
      memset(&write_ota_state, 0x00, sizeof(iot_state_t));

      write_ota_state.fota_state = FOTA_INSTALLATION_REQUESTED;
      /*
       * Store the current firmware version in FLASH.
       * This is used at next boot to determine if the installation procedure succeeded or not.
       * If the running firmware has the same version this means the installation procedure failed.
       * If the running firmware has a different version then it means the installation procedure succeeded.
       */
      snprintf(write_ota_state.prev_fw_version, IOT_STATE_FW_VERSION_LEN, "%d.%d.%d", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);

      ret = setIoTState(&write_ota_state);

      if (0 != ret)
      {
        msg_error("setIoTDeviceConfig(FOTA_INSTALLATION_REQUESTED) failed.\n");
      }
      else
      {
        msg_info("%s memorized as previous FW version.\n", write_ota_state.prev_fw_version);
        /* Reboot after everything is successfuly stored */
        printf("     Reboot\n");
        HAL_Delay(1000U);
        NVIC_SystemReset();
      }
    }
    else
    {
      msg_info("Problem in RFU update (%d)\n", ret);
    }

  }
#endif
}


/**
 * @brief   Return the integer difference between 'init + timeout' and 'now'.
 *          The implementation is robust to uint32_t overflows.
 * @param   In:   init      Reference index.
 * @param   In:   now       Current index.
 * @param   In:   timeout   Target index.
 * @retval  Number of units from now to target.
 */
int32_t comp_left_ms(uint32_t init, uint32_t now, uint32_t timeout)
{
  uint32_t elapsed = 0;

  if (now < init)
  { /* Timer wrap-around detected */
    /* printf("Timer: wrap-around detected from %d to %d\n", init, now); */
    elapsed = UINT32_MAX - init + now;
  }
  else
  {
    elapsed = now - init;
  }

  return timeout - elapsed;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
