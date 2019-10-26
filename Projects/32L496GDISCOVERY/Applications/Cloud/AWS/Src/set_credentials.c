/**
  ******************************************************************************
  * @file    set_credentials.c
  * @author  MCD Application Team
  * @brief   set the device connection credentials
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#include <stdint.h>
#include "main.h"
#include "iot_flash_config.h"
#include "net_connect.h"

/* Global variables ----------------------------------------------------------*/
extern int32_t cellular_net_driver(net_if_handle_t * pnetif);
net_if_driver_init_func device_driver_ptr=&cellular_net_driver;
const net_cellular_credentials_t  Credentials =
  {
    "EM",
    "",
    "",
    false,
  };
  

/* Function prototypes -----------------------------------------------*/
int32_t set_network_credentials(net_if_handle_t *pnetif);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Prompt and set the credentials for the network interface
  * @param  pnetif     Pointer to the network interface

  * @retval NET_OK               in case of success
  *         NET_ERROR_FRAMEWORK  if unable to set the parameters
  */
int32_t set_network_credentials(net_if_handle_t *pnetif)
{
  
  if (net_cellular_set_credentials(pnetif, &Credentials) != NET_OK)
  {
    msg_error("Can not set the cellular credentials\n");
    return NET_ERROR_FRAMEWORK;
  }
  return NET_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
