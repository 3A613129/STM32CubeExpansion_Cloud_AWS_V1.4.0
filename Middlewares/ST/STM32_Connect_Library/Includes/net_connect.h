/**
  ******************************************************************************
  * @file    net_connect.h
  * @author  MCD Application Team
  * @brief   Provides the network interface APIs.
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

#ifndef NET_CONNECT_H
#define NET_CONNECT_H
#include "stdbool.h"
#include "net_conf.h"
#include "net_address.h"
#include "net_errors.h"
#include "net_wifi.h"
#include "net_cellular.h"
#include "net_custom.h"

/* flags */

#define NET_ETHERNET_FLAG_DEFAULT_IF  1



/* Socket family */
#define NET_AF_INET           2
#define NET_AF_UNSPEC         0
#define NET_AF_INET6        NET_AF_UNSPEC /* Not supported */
#define NET_IPADDR_ANY        ((u32_t)0x00000000UL)

/* Socket types */
#define NET_SOCK_STREAM         1
#define NET_SOCK_DGRAM          2
#define NET_SOCK_RAW            3

/* Socket protocol */
#define NET_IPPROTO_TCP         6
#define NET_IPPROTO_ICMP        1
#define NET_IPPROTO_UDP        17
#define NET_IPPROTO_TCP_TLS    36


#define NET_SHUTDOWN_R          0
#define NET_SHUTDOWN_W          1
#define NET_SHUTDOWN_RW         2

#define NET_SOL_SOCKET                  0xfff

#define NET_SO_RCVTIMEO     0x1006
#define NET_SO_SNDTIMEO     0x1005


#define NET_SO_EVENT_CALLBACK           2
#define NET_SO_BINDTODEVICE             3
#define NET_SO_BLOCKING                 4
#define NET_SO_SECURE                   5

#define NET_SO_TLS_CA_CERT              7
#define NET_SO_TLS_CA_CRL               8
#define NET_SO_TLS_DEV_KEY              9
#define NET_SO_TLS_DEV_CERT             10
#define NET_SO_TLS_SERVER_VERIFICATION  11
#define NET_SO_TLS_SERVER_NAME          12
#define NET_SO_TLS_PASSWORD             13
#define NET_SO_TLS_CERT_PROF            14


#define NET_MSG_DONTWAIT      0x08    /* Nonblocking i/o for this operation only */

typedef struct pbuf net_buf_t;




typedef enum {
  NET_EVENT_STATE_CHANGE,
  NET_EVENT,
  NET_EVENT_WIFI,
} net_evt_t;


/** Network states. */
typedef enum {
  NET_STATE_DEINITIALIZED = 0,
  NET_STATE_INITIALIZED,

  NET_STATE_STARTING,
  NET_STATE_STARTED,
  NET_STATE_STOPPING,
  NET_STATE_STOPPED,

  NET_STATE_CONNECTING,
  NET_STATE_CONNECTED,
  NET_STATE_DISCONNECTING,
  NET_STATE_DISCONNECTED,
  NET_STATE_LINK_LOST,

  NET_STATE_ERROR,
} net_state_t;

/** Network events. */
typedef enum {
  NET_EVENT_POWERSAVE_ENABLED = 0,
} net_event_t;

/* Wi-Fi events */
typedef enum {
  WIFI_SCAN_RESULTS_READY
} net_wifi_event_t;


typedef struct net_if_drv_s             net_if_drv_t;
typedef struct net_ip_if_s              net_ip_if_t;

typedef void(* 	net_if_notify_func) (void *context, uint32_t event_class,uint32_t event_id, void  *event_data);

typedef struct {
 net_if_notify_func callback;
 void *context;
} net_event_handler_t;

 struct net_if_handle_s {
  struct net_if_handle_s *next;
  ipaddr_t       ipaddr;
  ipaddr_t       gateway;
  ipaddr_t       netmask;
  bool           dhcp_mode;
  char  DeviceName[NET_DEVICE_NAME_LEN];
  char  DeviceID  [NET_DEVICE_ID_LEN];
  char  DeviceVer [NET_DEVICE_VER_LEN];
  macaddr_t  macaddr;
  net_state_t    state;
  net_if_drv_t   *pdrv;
  net_ip_if_t    *net_ip_if;
  int32_t (*iplib_input_data)(void *context,  net_buf_t *p);
  void (*iplib_notify_link_change)(void *context,uint8_t status);

  const net_event_handler_t *event_handler;
} ;





typedef int32_t(* net_if_driver_init_func) (net_if_handle_t * pnetif);



/* network state control functions */
int32_t net_if_init (net_if_handle_t *pnetif, net_if_driver_init_func driver_init, const net_event_handler_t *event_handler);
int32_t net_if_deinit (net_if_handle_t *pnetif);

int32_t net_if_start (net_if_handle_t *pnetif);
int32_t net_if_stop (net_if_handle_t *pnetif);


int32_t net_if_connect (net_if_handle_t *pnetif);
int32_t net_if_disconnect (net_if_handle_t *pnetif);

int32_t net_if_powersave_enable(net_if_handle_t * pnetif);
int32_t net_if_powersave_disable(net_if_handle_t * pnetif);

int32_t net_if_getState (net_if_handle_t *pnetif, net_state_t *state);
int32_t net_if_wait_state(net_if_handle_t *pnetif, net_state_t state, uint32_t timeout);

/* network event management */
void net_if_notify(net_if_handle_t *pnetif,uint32_t event_class,uint32_t event_if, void  *event_data);

/* network parameter and status functions */
int32_t net_if_set_dhcp_mode(net_if_handle_t *pnetif,bool mode);
int32_t net_if_set_ipaddr(net_if_handle_t *pnetif,ipaddr_t ipaddr,ipaddr_t gateway,ipaddr_t netmask);
int32_t net_if_get_mac_address(net_if_handle_t *pnetif, macaddr_t * mac);
int32_t net_if_get_ip_address(net_if_handle_t *pnetif, ipaddr_t * ip);
int32_t net_if_gethostbyname(net_if_handle_t *pnetif, sockaddr_t *addr,char *name);
int32_t net_if_ping(net_if_handle_t *pnetif,sockaddr_t *addr,int count , int delay, int reponse[]);

//int32_t net_if_sleep(net_if_handle_t * pnetif);
//int32_t net_if_wakeup(net_if_handle_t * pnetif);




/* network socket API */
int32_t net_socket(int32_t Domain, int32_t Type, int32_t Protocol);
int32_t net_bind(int32_t sock, sockaddr_t *addr, int32_t addrlen );
int32_t net_accept(int32_t sock, sockaddr_t *addr, int32_t *addrlen );
int32_t net_closesocket(int32_t sock);
int32_t net_shutdown(int32_t sock,uint32_t mode);
int32_t net_setsockopt( int32_t sock, int32_t level, int32_t optname, const void *optvalue, int32_t optlen);
int32_t net_getsockopt( int32_t sock, int32_t level, int32_t optname, void *optvalue, int32_t *optlen);
int32_t net_connect(int32_t sock, sockaddr_t *addr, int32_t addrlen );
int32_t net_listen(int32_t sock, int32_t backlog);
int32_t net_send (int32_t sock, uint8_t *buf, int32_t len, int32_t flags);
int32_t net_recv (int32_t sock, uint8_t *buf, int32_t len, int32_t flags);
int32_t net_sendto (int32_t sock, uint8_t *buf, int32_t len, int32_t flags, sockaddr_t *to, int32_t tolen);
int32_t net_recvfrom (int32_t sock, uint8_t *buf, int32_t len, int32_t flags, sockaddr_t *from, int32_t *fromlen);
int32_t net_getsockname(int32_t sock,sockaddr_t *name, int32_t *namelen);
int32_t net_getpeername(int32_t sock,sockaddr_t *name, int32_t *namelen);


extern  const unsigned int net_tls_sizeof_suite_structure;
extern  const void    *net_tls_user_suite0;
extern  const void    *net_tls_user_suite1;
extern  const void    *net_tls_user_suite2;
extern  const void    *net_tls_user_suite3;
extern  const void    *net_tls_user_suite4;
#endif /* NET_CONNECT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
