/**
  ******************************************************************************
  * @file    net_ping.c
  * @author  MCD Application Team
  * @brief   application to send icmp ping
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

#include "lwip/tcpip.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/api.h" /* HAL_GetTick() */
#include "lwip/ip4.h"



/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/


/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif






/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static void ping_prepare_echo( struct icmp_echo_hdr *iecho, uint16_t len,uint16_t ping_seq_num)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id     = PING_ID;
  iecho->seqno  = net_htons(ping_seq_num);

  /* fill the additional data buffer with some data */
  for(i = 0; i < data_len; i++) {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

 //iecho->chksum = inet_chksum(iecho, len);
}

int32_t icmp_ping(net_if_handle_t *pnetif,sockaddr_t *addr,int count , int timeout, int response[])
{
      int32_t sock;
      int ping_start_time;
      int       ret=-1;
      sockaddr_t from;
      int32_t fromlen, len = 0;
      static int ping_seq_num=1;
      char buf[64]="";
      struct ip_hdr *iphdr;
      struct icmp_echo_hdr *iecho,*pecho;
      size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

      NET_DBG_INFO("ping %s %i bytes\n", net_ntoa(&((sockaddr_in*)addr)->sin_addr),PING_DATA_SIZE);

      if ((sock = net_socket(NET_AF_INET, NET_SOCK_RAW, NET_IPPROTO_ICMP)) < 0)
      {
          NET_DBG_ERROR("ping: socket fail\n\r");
          return -1;
      }

#if 0
      if (net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_BINDTODEVICE, pnetif, sizeof(void*))<0)
      {
          NET_DBG_ERROR("ping: setsockopt() bind to network addapater fail\n\r");
          return -1;
      }
#endif
      if (net_setsockopt(sock, NET_SOL_SOCKET, NET_SO_RCVTIMEO, &timeout, sizeof(timeout))<0)
      {
          NET_DBG_ERROR("ping: setsockopt() fail\n\r");
          return -1;
      }

      pecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
      if (!pecho) {
          NET_DBG_ERROR("ping_client_process : message alloc fails\n\r");
          return -1;
      }


      ((sockaddr_in*)addr)->sin_family      = NET_AF_INET;
      ((sockaddr_in*)addr)->sin_port        = net_htons(NET_IPADDR_ANY);


      for(int i=0;i<count;i++)
      {
        response[i]=-1;
        ping_prepare_echo(pecho, (uint16_t) ping_size, ping_seq_num );

        if(net_sendto(sock, (uint8_t*)pecho, ping_size, 0,addr, sizeof(sockaddr_t)) < 0)
        {
          NET_DBG_INFO("ping_client_process : send fail\n\r");
          break;
        }

        ping_start_time=HAL_GetTick();
        do
        {

          len = net_recvfrom(sock, (uint8_t*)buf, ping_size, 0, &from, &fromlen);
          if (len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr)))
          {
            iphdr = (struct ip_hdr *)buf;
            iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
            if ((iecho->id == PING_ID) && (iecho->seqno == net_htons(ping_seq_num)))
            {
              if (ICMPH_TYPE(iecho) == ICMP_ER)
              {
                ret = 0;
                response[i]= HAL_GetTick()- ping_start_time;
                break;
              }
              else
              {
                NET_DBG_ERROR("ICMP Other Response received \n\r");
              }
            }
          }
          else
          {
            int now=HAL_GetTick();
            NET_DBG_ERROR("no data start : %d  .. %d\n\r",ping_start_time,now);
          }
        }
        while (HAL_GetTick() <( ping_start_time + timeout));
        ping_seq_num++;
      }
      mem_free(pecho);
      net_closesocket(sock);
      return ret;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
