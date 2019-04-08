#ifndef __USBD_CDC_CAT_IF_H__
#define __USBD_CDC_CAT_IF_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_ua3reo.h"

extern USBD_CAT_ItfTypeDef USBD_CAT_fops_FS;
uint8_t CAT_Transmit_FS(uint8_t* Buf, uint16_t Len);
void CAT_Transmit(char* data);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_IF_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
