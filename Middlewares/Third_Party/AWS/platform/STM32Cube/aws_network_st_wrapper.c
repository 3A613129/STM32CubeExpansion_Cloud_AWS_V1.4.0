/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>
#include <timer_platform.h>
#include <network_interface.h>
#include "aws_iot_error.h"
#include "aws_iot_log.h"
#include "network_platform.h"
#include "heap.h"
#include "net_connect.h"
#include "msg.h"

void _iot_tls_set_connect_params(Network *pNetwork, char *pRootCALocation, char *pDeviceCertLocation,
                                 char *pDevicePrivateKeyLocation, char *pDestinationURL,
                                 uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag);


/* This is the value used for ssl read timeout */
#define IOT_SSL_READ_TIMEOUT 10


void _iot_tls_set_connect_params(Network *pNetwork, char *pRootCALocation, char *pDeviceCertLocation,
                                 char *pDevicePrivateKeyLocation, char *pDestinationURL,
                                 uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag)
{
  pNetwork->tlsConnectParams.DestinationPort = destinationPort;
  pNetwork->tlsConnectParams.pDestinationURL = pDestinationURL;
  pNetwork->tlsConnectParams.pDeviceCertLocation = pDeviceCertLocation;
  pNetwork->tlsConnectParams.pDevicePrivateKeyLocation = pDevicePrivateKeyLocation;
  pNetwork->tlsConnectParams.pRootCALocation = pRootCALocation;
  pNetwork->tlsConnectParams.timeout_ms = timeout_ms;
  pNetwork->tlsConnectParams.ServerVerificationFlag = ServerVerificationFlag;
}

IoT_Error_t iot_tls_init(Network *pNetwork, char *pRootCALocation, char *pDeviceCertLocation,
                         char *pDevicePrivateKeyLocation, char *pDestinationURL,
                         uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag)
{
  _iot_tls_set_connect_params(pNetwork, pRootCALocation, pDeviceCertLocation, pDevicePrivateKeyLocation,
                              pDestinationURL, destinationPort, timeout_ms, ServerVerificationFlag);

  pNetwork->connect = iot_tls_connect;
  pNetwork->read = iot_tls_read;
  pNetwork->write = iot_tls_write;
  pNetwork->disconnect = iot_tls_disconnect;
  pNetwork->isConnected = iot_tls_is_connected;
  pNetwork->destroy = iot_tls_destroy;
  return SUCCESS;
}

IoT_Error_t iot_tls_is_connected(Network *pNetwork)
{
  /* Use this to add implementation which can check for physical layer disconnect */
  return NETWORK_PHYSICAL_LAYER_CONNECTED;
}





IoT_Error_t iot_tls_connect(Network *pNetwork, TLSConnectParams *params)
{
  if(NULL == pNetwork)
  {
    return NULL_VALUE_ERROR;
  }

  if(NULL != params)
  {
    _iot_tls_set_connect_params(pNetwork, params->pRootCALocation, params->pDeviceCertLocation,
                                params->pDevicePrivateKeyLocation, params->pDestinationURL,
                                params->DestinationPort, params->timeout_ms, params->ServerVerificationFlag);
  }

  int ret = 0;
  int32_t sock;
  sockaddr_in addr;

  sock = net_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
  if (sock < 0)
  {
    msg_error(" failed to create a TCP socket  ! net_socket returned %d\n", (int) sock);
    return SSL_CONNECTION_ERROR;
  }

  addr.sin_len    = sizeof(sockaddr_in);

  if ((ret |= net_if_gethostbyname(NULL,(sockaddr_t *)&addr,pNetwork->tlsConnectParams.pDestinationURL)) != NET_OK)
  {
    msg_error("Could not find hostname ipaddr %s\n",pNetwork->tlsConnectParams.pDestinationURL);
  }
  else
  {
    bool false_val = false;
    bool true_val = true;
    uint32_t timeout=IOT_SSL_READ_TIMEOUT;

    ret  = net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_SECURE, NULL, 0);
    ret |= net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_CERT_PROF, net_tls_user_suite0, net_tls_sizeof_suite_structure);
    ret |= net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_RCVTIMEO, (void *) &timeout, sizeof(uint32_t));
    ret |= net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_CA_CERT, (void *)pNetwork->tlsConnectParams.pRootCALocation, strlen(pNetwork->tlsConnectParams.pRootCALocation) + 1);
    ret |= net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_DEV_CERT, (void *)pNetwork->tlsConnectParams.pDeviceCertLocation, strlen(pNetwork->tlsConnectParams.pDeviceCertLocation) + 1);
    ret |= net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_DEV_KEY, (void *)pNetwork->tlsConnectParams.pDevicePrivateKeyLocation, strlen(pNetwork->tlsConnectParams.pDevicePrivateKeyLocation) + 1);
    ret |= net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_SERVER_NAME, (void *)pNetwork->tlsConnectParams.pDestinationURL, strlen(pNetwork->tlsConnectParams.pDestinationURL) + 1);
    ret |= net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_TLS_SERVER_VERIFICATION,(pNetwork->tlsConnectParams.ServerVerificationFlag == true) ? (void *) &false_val : (void *) &true_val, sizeof(bool));

    pNetwork->tlsDataParams.server_fd.fd = sock;
    addr.sin_port = net_htons(pNetwork->tlsConnectParams.DestinationPort);

    if (ret |= net_connect(sock, (sockaddr_t *)&addr, sizeof(addr)) != NET_OK)
    {
      msg_error("Could not open the socket at %s port %d.\n", pNetwork->tlsConnectParams.pDestinationURL, pNetwork->tlsConnectParams.DestinationPort);
    }

  }

  return (IoT_Error_t) ret;
}

IoT_Error_t iot_tls_write(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *timer, size_t *written_len)
{
  IoT_Error_t rc = SUCCESS;
  size_t written_so_far = 0;
  int ret = 0;
  bool bTimerExpired = false;

  do {

    ret = net_send(pNetwork->tlsDataParams.server_fd.fd, pMsg + written_so_far, len - written_so_far, 0);

    if (ret >= 0)
    {
      written_so_far += ret;
    }
    else
    {
      switch(ret)
      {
        case MBEDTLS_ERR_SSL_WANT_READ:
        case MBEDTLS_ERR_SSL_WANT_WRITE:
          break;
        case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
        case MBEDTLS_ERR_SSL_CONN_EOF:
          rc = NETWORK_DISCONNECTED_ERROR;
          break;
        default:
          msg_error(" failed\n  ! mbedtls_ssl_write returned -0x%x\n\n", -ret);
          /* All other negative return values indicate connection needs to be reset.
           * Will be caught in ping request so ignored here */
          rc = NETWORK_SSL_WRITE_ERROR;
      }
    }
    bTimerExpired = has_timer_expired(timer);
  } while ( (rc == SUCCESS) && (written_so_far < len) && !bTimerExpired );

  *written_len = written_so_far;
  if ((*written_len != len) && bTimerExpired)
  {
    rc = NETWORK_SSL_WRITE_TIMEOUT_ERROR;
  }

  return rc;
}



IoT_Error_t iot_tls_read(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *timer, size_t *read_len)
{
  size_t rxLen = 0;
  int ret = 0;

  while (len > 0) {
    /* This read will timeout after IOT_SSL_READ_TIMEOUT if there's no data to be read */
    ret = net_recv(pNetwork->tlsDataParams.server_fd.fd, pMsg, len, 0);

    if(ret == NET_TIMEOUT)
    {
      ret = 0;
    }

    if (ret > 0) {
      rxLen += ret;
      pMsg += ret;
      len -= ret;
    }
    else
    {
      if(ret < 0)
      {
        msg_error("net_recv failed - %d\n", ret);
        return FAILURE;
      }
    }

    /* Evaluate timeout after the read to make sure read is done at least once */
    if (has_timer_expired(timer))
    {
      break;
    }
  }

  if (len == 0)
  {
    *read_len = rxLen;
    return SUCCESS;
  }

  if (rxLen == 0)
  {
    return NETWORK_SSL_NOTHING_TO_READ;
  }
  else
  {
    return NETWORK_SSL_READ_TIMEOUT_ERROR;
  }
}

IoT_Error_t iot_tls_disconnect(Network *pNetwork)
{
  if (NET_OK !=  net_closesocket(pNetwork->tlsDataParams.server_fd.fd))
  {
    msg_error("net_closesocket() failed.\n");
  }

  return SUCCESS;
}

IoT_Error_t iot_tls_destroy(Network *pNetwork)
{
  return SUCCESS;
}

#ifdef __cplusplus
}
#endif
