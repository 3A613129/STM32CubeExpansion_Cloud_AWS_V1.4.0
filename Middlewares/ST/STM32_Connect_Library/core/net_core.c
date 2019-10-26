/**
  ******************************************************************************
  * @file    net_core.c
  * @author  MCD Application Team
  * @brief   Network interface core implementation
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

static void netif_add_to_list(net_if_handle_t *pnetif);
static void netif_remove_from_list(net_if_handle_t *pnetif);
static net_if_handle_t *netif_list = NULL;


static void netif_add_to_list(net_if_handle_t *pnetif)
{
  LOCK_NETIF_LIST();
  if(netif_list == NULL)
  {
    netif_list = pnetif;
  }
  else
  {
    // add it to end of the list
    net_if_handle_t *plastnetif;
    plastnetif = netif_list;
    while (plastnetif->next) plastnetif = plastnetif->next;
    plastnetif->next = pnetif;
  }
  UNLOCK_NETIF_LIST();
}


static void netif_remove_from_list(net_if_handle_t *pnetif)
{
  net_if_handle_t       *pnetif_prev;
  LOCK_NETIF_LIST();

  if(netif_list == pnetif)
  {
    netif_list = netif_list->next;
  }
  else
  {
    for(pnetif_prev = netif_list; pnetif_prev->next != NULL; pnetif_prev = pnetif_prev->next)
    {
      if(pnetif_prev->next == pnetif)
      {
        pnetif_prev->next = pnetif->next;
        break;
      }
    }
  }
  UNLOCK_NETIF_LIST();
}



/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
net_if_handle_t * net_if_find (ipaddr_t  addr)
{
  net_if_handle_t *ptr;

  LOCK_NETIF_LIST();

  ptr = netif_list;

  if(addr != 0)
  {
    do
    {
      if (ptr->ipaddr == addr)
      {
        break;
      }
      ptr = ptr->next;
    } while(ptr);
  }
  UNLOCK_NETIF_LIST();
  return ptr;
}


net_if_handle_t *netif_check(net_if_handle_t *pnetif)
{
  if (pnetif == NULL)
  {
    // get default interface
    pnetif = net_if_find(0);
    if (pnetif == NULL)
    {
      NET_DBG_ERROR ("No network interface defined");
    }
  }
  return pnetif;
}

static bool check_state_transition(net_state_t to,net_state_t from)
{

 // FIXME later when state are properly defined

 return true;
}

int32_t net_if_wait_state(net_if_handle_t *pnetif, net_state_t state, uint32_t timeout)
{
  volatile net_state_t      *p;
  p=&pnetif->state;
  while(*p != state)
  {
    WAIT_STATE_CHANGE(timeout);
  }
  return NET_OK;

}




void net_if_notify(net_if_handle_t *pnetif,uint32_t event_class,uint32_t event_id, void  *event_data)
{
   // call the user Handler first ,FIXME , first or not , race between wait state transition and user handler
  if ((NULL != pnetif->event_handler) && (NULL != pnetif->event_handler->callback))
  {
    pnetif->event_handler->callback(pnetif->event_handler->context,event_class,event_id,event_data);
  }
  
  switch(event_class)
  {
    case NET_EVENT_STATE_CHANGE:
      pnetif->state = (net_state_t) (uint32_t) event_id;
      SIGNAL_STATE_CHANGE();
      break;

    default:
      break;
  }

 
}

#ifdef NET_USE_RTOS
static int net_initialized =0;
#endif
/**

  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_init (net_if_handle_t *pnetif, net_if_driver_init_func driver_init ,const net_event_handler_t *event_handler)
{
  int32_t ret = NET_ERROR_FRAMEWORK;
#ifdef NET_USE_RTOS
  if (net_initialized ==0)
  {
    net_init_locks();
  }
#endif
  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    pnetif->event_handler = event_handler;
    pnetif->state = NET_STATE_INITIALIZED;
    netif_add_to_list(pnetif);
    ret = (*driver_init)(pnetif);
    if (NET_OK != ret)
    {
      NET_DBG_ERROR ("Interface cannot be initialized.");
      return NET_ERROR_INTERFACE_FAILURE;
    }
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}

/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_deinit (net_if_handle_t *pnetif)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if(check_state_transition(pnetif->state,NET_STATE_DEINITIALIZED))
    {
      ret = pnetif->pdrv->if_deinit(pnetif);
      if (ret != NET_OK)
      {
        NET_DBG_ERROR ("Interface cannot be de-initialized");
        ret = NET_ERROR_STATE_TRANSITION;
      }
      else
      {
       netif_remove_from_list(pnetif);
      }
    }
    else
    {
      NET_DBG_ERROR ("Incorrect requested State transition");
      ret = NET_ERROR_INVALID_STATE_TRANSITION;
    }
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}

/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_start (net_if_handle_t *pnetif)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if(check_state_transition(pnetif->state,NET_STATE_INITIALIZED))
    {
      ret = pnetif->pdrv->if_start(pnetif);
      if (NET_OK != ret)
      {
        NET_DBG_ERROR ("Interface cannot be started.");
        return NET_ERROR_INTERFACE_FAILURE;
      }
    }
    else
    {
      NET_DBG_ERROR ("Incorrect requested State transition");
      ret = NET_ERROR_INVALID_STATE_TRANSITION;
    }
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}

/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_stop (net_if_handle_t *pnetif)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if(check_state_transition(pnetif->state,NET_STATE_STARTED))
    {
      ret = pnetif->pdrv->if_stop(pnetif);
      if (ret != NET_OK)
      {
        NET_DBG_ERROR ("Interface cannot be stopped");
        ret = NET_ERROR_STATE_TRANSITION;
      }
    }
    else
    {
      NET_DBG_ERROR ("Incorrect requested State transition");
      ret = NET_ERROR_INVALID_STATE_TRANSITION;
    }
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}


/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */

int32_t net_if_connect (net_if_handle_t *pnetif)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if(check_state_transition(pnetif->state,NET_STATE_CONNECTED))
    {
      ret = pnetif->pdrv->if_connect(pnetif);
      if (ret != NET_OK)
      {
        NET_DBG_ERROR ("Interface cannot be connected");
        ret = NET_ERROR_STATE_TRANSITION;
      }
    }
    else
    {
      NET_DBG_ERROR ("Incorrect requested State transition");
      ret = NET_ERROR_INVALID_STATE_TRANSITION;
    }
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}

/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_disconnect (net_if_handle_t *pnetif)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if(check_state_transition(pnetif->state,NET_STATE_DISCONNECTED))
    {
      ret = pnetif->pdrv->if_disconnect(pnetif);
    }
    else
    {
      NET_DBG_ERROR ("Incorrect requested State transition");
      ret = NET_ERROR_INVALID_STATE_TRANSITION;
    }
  }
  else
  {
    ret = NET_ERROR_PARAMETER;
    NET_DBG_ERROR ("Invalid interface.");
  }
  return ret;
}


/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */

int32_t net_if_powersave_enable (net_if_handle_t *pnetif)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if(pnetif->state == NET_STATE_CONNECTED)
    {
      ret = pnetif->pdrv->if_powersave_enable(pnetif);
    }
    else
    {
      NET_DBG_ERROR ("Power-save cannot be enabled when the device is not connected");
      ret = NET_ERROR_INVALID_STATE_TRANSITION;
    }
  }
  else
  {
    ret = NET_ERROR_PARAMETER;
    NET_DBG_ERROR ("Invalid interface.");
  }
  return ret;
}


/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_get_mac_address(net_if_handle_t *pnetif, macaddr_t * mac)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if( NET_STATE_DEINITIALIZED != pnetif->state )
    {
      memcpy(mac, &pnetif->macaddr, sizeof(macaddr_t));
      ret = NET_OK;
    }
    else
    {
      ret = NET_ERROR_INTERFACE_FAILURE;
      NET_DBG_ERROR ("Interface not yet initialized or in error state");
    }
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}

/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_get_ip_address(net_if_handle_t *pnetif, ipaddr_t * ip)
{
  int32_t ret = NET_ERROR_FRAMEWORK;

  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    if(pnetif->state == NET_STATE_CONNECTED)
    {
      *ip = pnetif->ipaddr;
    }
    else
    {
      NET_DBG_ERROR ("Can get ipaddr for un connected network interface");
      ret = NET_ERROR_INTERFACE_FAILURE;
    }
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}

/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_getState (net_if_handle_t *pnetif, net_state_t *state)
{
  int32_t ret = NET_ERROR_FRAMEWORK;
  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    *state = pnetif->state;
    ret = NET_OK;
  }
  else
  {
    NET_DBG_ERROR ("Invalid interface.");
    ret = NET_ERROR_PARAMETER;
  }
  return ret;
}



/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_gethostbyname(net_if_handle_t *pnetif, sockaddr_t *addr,char *name)
{
  int32_t ret = NET_ERROR_FRAMEWORK;
  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
   ret =  pnetif->pdrv->gethostbyname(pnetif,addr,name);
  }
  return ret;
}

int32_t net_if_ping(net_if_handle_t *pnetif,sockaddr_t *addr,int count , int delay, int response[])
{
  int32_t ret = NET_ERROR_FRAMEWORK;
  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    ret =  pnetif->pdrv->ping(pnetif,addr,count,delay,response);
  }
  return ret;
}

int32_t net_if_set_dhcp_mode(net_if_handle_t *pnetif, bool mode)
{
  int32_t ret = NET_ERROR_FRAMEWORK;
  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    pnetif->dhcp_mode=mode;
    ret = NET_OK;
  }
  return ret;
}

/**
  * @brief  Function description
  * @param  Params
  * @retval socket status
  */
int32_t net_if_set_ipaddr(net_if_handle_t *pnetif, ipaddr_t ipaddr, ipaddr_t gateway, ipaddr_t netmask)
{
  int32_t ret = NET_ERROR_FRAMEWORK;
  pnetif = netif_check(pnetif);
  if(pnetif != NULL)
  {
    pnetif->ipaddr  = ipaddr;
    pnetif->gateway = gateway;
    pnetif->netmask = netmask;
    ret = NET_OK;
  }
  return ret;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

