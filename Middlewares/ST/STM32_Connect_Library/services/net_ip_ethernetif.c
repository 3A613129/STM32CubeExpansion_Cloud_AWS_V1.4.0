/**
  ******************************************************************************
  * @file    net_ip_ethernetif.c
  * @author  MCD Application Team
  * @brief   from net_ip to ethernet network interface functions
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
#include "net_ip_ethernet.h"
#include "net_buffers.h"

/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT ( 250 ) //( portMAX_DELAY )
/* Stack size of the interface thread */

#define INTERFACE_THREAD_STACK_SIZE 1024
#define BUFFER_LIST_SIZE        10

#define CIRCULAR_INC(a) a++;if (a==BUFFER_LIST_SIZE) a=0

static volatile int  free_buffer_index;
static int     write_buffer_index;
static int     busy_buffer_index;
static net_buf_t *sent_write_buffer[BUFFER_LIST_SIZE];

static uint8_t link_status = 0;

/* Semaphore to signal incoming packets */
static osSemaphoreId s_xSemaphore = NULL;
static osThreadId ethernetif_thread_handle = NULL;
static  void ethernetif_task(void const *argument);
static  void event_callback(void);
static  void ethernetif_write_done(uint8_t *);


////////////////////////////////////////// SRA CODE FROM HERE ON
typedef struct
{
  int32_t (*input)(void *context, net_buf_t *p);
  void (*link_status)(void *context,uint8_t status);
  void *context;
} ethernetif_cb_t;


void net_ethernetif_get_mac_addr(uint8_t *MACAddr_in)
{
  ethernetif_low_get_mac_addr(MACAddr_in);
  return;
}

static  void ethernetif_task(void const *argument)
{
  int32_t semaphore_retval;
  net_buf_t   *netbuf;
  uint8_t     *payload;
  int16_t len;

  ethernetif_cb_t *cb = (ethernetif_cb_t *) argument;
  link_status = ethernetif_low_get_link_status();
  if(cb->link_status)
        cb->link_status(cb->context,link_status);

  for( ;; )
  {
    semaphore_retval = osSemaphoreWait(s_xSemaphore, TIME_WAITING_FOR_INPUT);
    if(ethernetif_low_get_link_status() != link_status)
    {
      link_status = !link_status;
      if(cb->link_status)
        cb->link_status(cb->context,link_status);
    }
    if (semaphore_retval == osOK)
    {
      while ((len = ethernetif_low_inputsize()) > 0)
      {
        payload = NULL;
        netbuf = net_buf_alloc(len);
        if (netbuf != NULL)
        {
          payload = netbuf->payload;
        }     
        if (ethernetif_low_input(payload, len) > 0 )
        {
          cb->input(cb->context, netbuf);
        }
      }
    }
  }
}

static  void event_callback()
{
  osSemaphoreRelease(s_xSemaphore);
}

void net_ethernetif_init(void *context, int32_t (*ethernetif_rx_cb)(void *context,net_buf_t *p), void (*ethernetif_link_status_cb)(void *context,uint8_t status))
{
  static ethernetif_cb_t cb;

  cb.input = ethernetif_rx_cb;
  cb.link_status = ethernetif_link_status_cb;
  cb.context=context;

 free_buffer_index =0;
 write_buffer_index = 0;
 busy_buffer_index = 0;
 memset(sent_write_buffer, 0, sizeof(sent_write_buffer));

/* create a  semaphore used for informing ethernetif of frame reception */
/* ethernet link list is 4 buffers, set semaphore size to 4 to not miss */
/* interrupt (1 is not working )                                        */
  osSemaphoreDef(SEM);
  s_xSemaphore = osSemaphoreCreate(osSemaphore(SEM) , 4);

  ethernetif_low_init(event_callback,ethernetif_write_done);


/* create the task that handles the ETH_MAC */
  osThreadDef(EthIf, ethernetif_task, osPriorityHigh, 0, INTERFACE_THREAD_STACK_SIZE);
  ethernetif_thread_handle = osThreadCreate (osThread(EthIf), &cb);


/* USER CODE BEGIN LOW_LEVEL_INIT */

/* USER CODE END LOW_LEVEL_INIT */
}

void net_ethernetif_deinit(void)
{
  osThreadTerminate(ethernetif_thread_handle);
  ethernetif_thread_handle = NULL;

  osSemaphoreDelete(s_xSemaphore);
  s_xSemaphore = NULL;

  ethernetif_low_deinit();
}



int32_t net_ethernetif_output(net_buf_t *netbuf)
{
 int32_t        ret;
 int32_t        l=0;
 int32_t        n=0;
 net_buf_t      *q;

  /* release returned buffers */
 while(busy_buffer_index != free_buffer_index)
 {
   CIRCULAR_INC(busy_buffer_index);
   net_buf_free(sent_write_buffer[busy_buffer_index]);
   sent_write_buffer[busy_buffer_index]=0;
 }

 for (q = netbuf; q ; q = q->next)
 {
   l += q->len;
   n++;
 }

 if (n==1)
 {
  net_buf_ref(netbuf);
  CIRCULAR_INC(write_buffer_index);
  sent_write_buffer[write_buffer_index]=netbuf;
  ret = ethernetif_low_output(netbuf->payload,netbuf->len);
 }
 else
 {
   net_buf_t    *p;
   p=net_buf_alloc(l);
   if (p)
   {
     uint8_t     *pp= p->payload;
     for (q = netbuf; q ; q = q->next)
     {
       memcpy(pp,q->payload,q->len);
       pp+= q->len;
     }
     p->len = l;
     CIRCULAR_INC(write_buffer_index);
     sent_write_buffer[write_buffer_index]=p;
     ret = ethernetif_low_output(p->payload,p->len);
   }
   else
   {
     while(1);
   }
 }
 return ret;
}

static void ethernetif_write_done(uint8_t *buffaddr)
{
 CIRCULAR_INC(free_buffer_index);
}

