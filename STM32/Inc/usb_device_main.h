/**
  ******************************************************************************
  * @file    usb_device.h
  * @author  Benedek Kupper
  * @version 0.1
  * @date    2018-11-03
  * @brief   USBDevice demo
  *
  * Copyright (c) 2018 Benedek Kupper
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#ifndef __USB_DEVICE_H_
#define __USB_DEVICE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <usbd.h>
#include <usbd_cdc.h>
#include <usbd_audio.h>

extern USBD_HandleType *const UsbDevice;
extern USBD_CDC_IfHandleType *const ua3reo_dev_debug_key_if;
extern USBD_AUDIO_IfHandleType *const ua3reo_dev_audio_if;
void USBD_CDC_Debug_Transmit_FIFO(USBD_CDC_IfHandleType *itf, uint8_t *data, uint16_t length);
void USBD_CDC_Debug_Transmit_FIFO_Events(USBD_CDC_IfHandleType *itf);

extern void HAL_USBD_Setup(void);
void UsbDevice_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_DEVICE_H_ */
