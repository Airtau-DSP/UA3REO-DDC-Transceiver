/**
  ******************************************************************************
  * @file    usb_device.c
  * @author  Benedek Kupper
  * @version 0.1
  * @date    2018-11-03
  * @brief   USB device definition and initialization
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
#include <usb_device_main.h>
#include <usbd_cdc.h>
#include <usbd_audio.h>

/** @brief USB device configuration */
const USBD_DescriptionType hdev_cfg = {
    .Vendor = {
        .Name           = "UA3REO",
        .ID             = 0x0483,
    },
    .Product = {
        .Name           = "UA3REO Transceiver",
        .ID             = 0xF001,
        .Version.bcd    = 0x0100,
    },
    .Config = {
        .Name           = "UA3REO Transceiver config",
        .MaxCurrent_mA  = 500,
        .RemoteWakeup   = 0,
        .SelfPowered    = 1,
    },
}, *const dev_cfg = &hdev_cfg;

USBD_HandleType hUsbDevice, *const UsbDevice = &hUsbDevice;

extern USBD_CDC_IfHandleType *const ua3reo_dev_debug_key_if;
extern USBD_CDC_IfHandleType *const ua3reo_dev_cat_if;
extern USBD_AUDIO_IfHandleType *const ua3reo_dev_audio_if;

void UsbDevice_Init(void)
{
    /* All fields of Config have to be properly set up */
    ua3reo_dev_debug_key_if->Config.InEpNum  = 0x81;
    ua3reo_dev_debug_key_if->Config.OutEpNum = 0x01;
    ua3reo_dev_debug_key_if->Config.NotEpNum = 0x82;
	
		ua3reo_dev_cat_if->Config.InEpNum  = 0x83;
    ua3reo_dev_cat_if->Config.OutEpNum = 0x03;
    ua3reo_dev_cat_if->Config.NotEpNum = 0x84;

		ua3reo_dev_audio_if->Config.OutEpNum  = 0x85;
		ua3reo_dev_audio_if->Config.InEpNum  = 0x86;
	
    /* Mount the interfaces to the device */
		//USBD_AUDIO_MountInterface(ua3reo_dev_audio_if, UsbDevice);
    USBD_CDC_MountInterface(ua3reo_dev_debug_key_if, UsbDevice);
		USBD_CDC_MountInterface(ua3reo_dev_cat_if, UsbDevice);

    /* Initialize the device */
    USBD_Init(UsbDevice, dev_cfg);

    /* The device connection can be made */
    USBD_Connect(UsbDevice);
}
