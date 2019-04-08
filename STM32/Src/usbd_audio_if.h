#ifndef __USBD_AUDIO_IF_H__
#define __USBD_AUDIO_IF_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_ua3reo.h"

extern USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops_FS;

void TransferComplete_CallBack_FS(void);
void HalfTransfer_CallBack_FS(void);
#ifdef __cplusplus
}
#endif

#endif /* __USBD_AUDIO_IF_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
