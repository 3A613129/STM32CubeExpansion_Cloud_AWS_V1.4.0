#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__
#include "net_connect.h"
#include "net_internals.h"

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Exported functions ------------------------------------------------------- */
#include "stm32f7xx_hal.h"
void net_ethernetif_init( void *context, int32_t (*ethernetif_rx_cb)(void *context,  net_buf_t *p), void (*ethernetif_link_status_cb)(void *context,uint8_t status));
void net_ethernetif_deinit(void);
void net_ethernetif_get_mac_addr(uint8_t *mac_addr);
uint8_t net_ethernetif_get_link_status(void);
int32_t net_ethernetif_output(net_buf_t *net_buf);
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
