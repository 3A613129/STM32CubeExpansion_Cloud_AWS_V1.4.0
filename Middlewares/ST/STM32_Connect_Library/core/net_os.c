/**
  ******************************************************************************
  * @file    net_os.c
  * @author  MCD Application Team
  * @brief   OS needed functions implementation
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
#include "stdarg.h"


#ifdef NET_USE_RTOS
extern void *pxCurrentTCB;


static osSemaphoreDef_t mutex_def[NET_LOCK_NUMBER]={0};
static osSemaphoreId   net_mutex[NET_LOCK_NUMBER];

void net_init_locks(void)
{
#ifdef NET_MBEDTLS_HOST_SUPPORT
  net_tls_init();
#endif
  for(int i=0;i<NET_LOCK_NUMBER;i++)
  {
    net_mutex[i]= osSemaphoreCreate(&mutex_def[i],1);
    NET_ASSERT(net_mutex[i]>0,"Failed on mutex creation");
  }
}

void net_destroy_locks(void)
{
#ifdef NET_MBEDTLS_HOST_SUPPORT
  net_tls_destroy();
#endif
  for(int i=0;i<NET_LOCK_NUMBER;i++)
  {
    osSemaphoreDelete(net_mutex[i]);
  }
}

void net_lock(int32_t sock,uint32_t t)
{
  int   ret;
  if (t ==NET_OS_WAIT_FOREVER) t = osWaitForever;
  ret = osSemaphoreWait(net_mutex[sock],t);
  NET_ASSERT(ret == 0 ,"Failed locking mutex");
}

void net_unlock(int32_t sock)
{
    int   ret;
    ret = osSemaphoreRelease(net_mutex[sock]);
    NET_ASSERT(ret == 0 ,"Failed unlocking mutex");
}

void net_lock_nochk(int32_t sock,uint32_t t)
{
  if (t ==NET_OS_WAIT_FOREVER) t = osWaitForever;
  osSemaphoreWait(net_mutex[sock],t);
}

void net_unlock_nochk(int32_t sock)
{
  osSemaphoreRelease(net_mutex[sock]);
}


#if !defined (__CC_ARM)	/* With MDK-ARM MICROLIB, printf() redefinition clashes with printfa.o */
int printf(const char *format,...)
{
  int rc;
  if ( xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )   vTaskSuspendAll();
  va_list arg;
  va_start(arg, format);
  rc = vprintf(format,arg);
  va_end(arg);
  if ( xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )     xTaskResumeAll();
  return rc;
}
#endif /* __CC_ARM */
void    *net_calloc(size_t n,size_t m)
{
  void *p;
  p = net_malloc(n*m);
  if (p) memset(p,0,n*m);
  return p;
}

void *malloc(size_t n)
{
  //NET_DBG_PRINT("calling malloc function");
  return pvPortMalloc(n);
}

void free(void *p)
{
 // NET_DBG_PRINT("calling free function");
  vPortFree(p);
}

/* needed for IAR EWARM heap management */
void *realloc(void *ptr, size_t size)
{
  if (ptr == NULL)
  {
    if (size != 0)
    {
      return malloc(size);
    }
    else
    {
      return NULL;
    }
  }
  else
  {
    if (size != 0)
    {
      void * new_ptr = NULL;
      new_ptr = malloc(size);
      if (new_ptr != NULL)
      {
        memcpy(new_ptr, ptr, size);
        free(ptr);
        return new_ptr;
      }
      else
      {
        return NULL;
      }
    }
    else
    {
      free(ptr);
      return NULL;
    }
  }
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

