#ifndef __USBD_AUDIO_IF_H__
#define __USBD_AUDIO_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

	/* Includes ------------------------------------------------------------------*/
#include "usbd_ua3reo.h"
#include "functions.h"

	extern USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops_FS;

	extern int16_t USB_AUDIO_rx_buffer_a[(AUDIO_BUFFER_SIZE / 2)];
	extern int16_t USB_AUDIO_rx_buffer_b[(AUDIO_BUFFER_SIZE / 2)];
	extern bool USB_AUDIO_current_rx_buffer; // a-false b-true

	void TransferComplete_CallBack_FS(void);
	void HalfTransfer_CallBack_FS(void);
	void AUDIO_GetRxBuffer_FS(USBD_HandleTypeDef *pdev);
#ifdef __cplusplus
}
#endif

#endif /* __USBD_AUDIO_IF_H__ */
