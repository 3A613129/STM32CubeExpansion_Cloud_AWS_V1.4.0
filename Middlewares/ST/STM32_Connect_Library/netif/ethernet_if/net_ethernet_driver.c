/**
  ******************************************************************************
  * @file    net_ethernet_driver.c
  * @author  MCD Application Team
  * @brief   Ethernet specific BSD-like socket wrapper
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

#include "net_connect.h"
#include "net_internals.h"
#include "net_ip_ethernet.h"

/* global constructor of the ethernet network interface */
int32_t ethernet_net_driver (net_if_handle_t * pnetif);




static int32_t net_ethernet_if_init (net_if_handle_t * pnetif);
static int32_t net_ethernet_if_deinit (net_if_handle_t * pnetif);
static int32_t net_ethernet_if_start (net_if_handle_t * pnetif);
static int32_t net_ethernet_if_stop (net_if_handle_t * pnetif);
static int32_t net_ethernet_if_connect (net_if_handle_t * pnetif);
static int32_t net_ethernet_if_disconnect (net_if_handle_t * pnetif );



/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */

extern iplib_t iplib_lwip;
static  iplib_t *iplib=&iplib_lwip;




int32_t ethernet_net_driver (net_if_handle_t * pnetif)
{
  int32_t ret;
  ret = net_ethernet_if_init(pnetif);
  return ret;
}

static int32_t  net_ethernet_if_init(net_if_handle_t * pnetif)
{
  int32_t ret = 0;
  net_if_drv_t *p = net_malloc(sizeof(net_if_drv_t));
  if ( p)
  {
    p->if_class=NET_INTERFACE_CLASS_ETHERNET;
    p->if_init=net_ethernet_if_init;
    p->if_deinit=net_ethernet_if_deinit;
    p->if_start=net_ethernet_if_start;
    p->if_stop=net_ethernet_if_stop;
    p->if_connect=net_ethernet_if_connect;
    p->if_disconnect=net_ethernet_if_disconnect;
    p->ping=icmp_ping;
    pnetif->pdrv = p;
    ret = NET_OK;
  }
  else
  {
    NET_DBG_ERROR("can't allocate memory for es_wifi_driver class\n");
    ret = NET_ERROR_NO_MEMORY;
  }
  return ret;
}

int32_t net_ethernet_if_start(net_if_handle_t * pnetif)
{
  int32_t ret = 0;

  net_if_notify(pnetif, NET_EVENT_STATE_CHANGE, NET_STATE_STARTING, NULL);

  // FIXME should not be marked as default one
  ret = iplib->add_if(pnetif,net_ethernetif_output, NET_ETHERNET_FLAG_DEFAULT_IF);
  if (ret == NET_OK)
  {
    net_ethernetif_init((void*) pnetif,pnetif->iplib_input_data, pnetif->iplib_notify_link_change);

    strncpy(pnetif->DeviceName,"Ethernet IF",NET_DEVICE_NAME_LEN);
    strncpy(pnetif->DeviceID,"Unknown",NET_DEVICE_ID_LEN);
    strncpy(pnetif->DeviceVer,"Unknown",NET_DEVICE_VER_LEN);
    net_ethernetif_get_mac_addr(pnetif->macaddr.mac);
    net_if_notify(pnetif, NET_EVENT_STATE_CHANGE, NET_STATE_STARTED, NULL);
  }
  return ret;
}



static int32_t net_ethernet_if_connect (net_if_handle_t * pnetif)
{
  int32_t       ret;
  ret =  iplib->connect(pnetif);
  return ret;
}

static int32_t net_ethernet_if_disconnect (net_if_handle_t * pnetif)
{
  return iplib->disconnect(pnetif);
}

static int32_t net_ethernet_if_stop (net_if_handle_t * pnetif)
{
  int32_t ret;
  ret = iplib->remove_if(pnetif);
  net_ethernetif_deinit();
  return ret;
}

static int32_t net_ethernet_if_deinit (net_if_handle_t * pnetif)
{
  int32_t ret = NET_OK;
  net_free(pnetif->pdrv);
  pnetif->pdrv = NULL;
  return ret;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/





