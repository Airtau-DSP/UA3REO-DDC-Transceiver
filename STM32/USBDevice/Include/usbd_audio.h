/**
  ******************************************************************************
  * @file    usbd_audio.h
  * @author  MCD Application Team
  * @brief   header file for the usbd_audio.c file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      http://www.st.com/SLA0044
  *
  ******************************************************************************
  */

  /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_AUDIO_H
#define __USB_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

	/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <usbd_types.h>
//#include  "usbd_ioreq.h"

/** @defgroup USBD_AUDIO_Exported_Defines
  * @{
  */

#define LOBYTE(x)  ((uint8_t)(x & 0x00FFU))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00U) >> 8U))

#ifndef USBD_AUDIO_FREQ
  /* AUDIO Class Config */
#define USBD_AUDIO_FREQ                               48000U
#endif /* USBD_AUDIO_FREQ */

#ifndef USBD_MAX_NUM_INTERFACES
#define USBD_MAX_NUM_INTERFACES                       1U
#endif /* USBD_AUDIO_FREQ */

#define AUDIO_DESCRIPTOR_TYPE                         0x21U
#define USB_DEVICE_CLASS_AUDIO                        0x01U
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01U
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02U
#define AUDIO_PROTOCOL_UNDEFINED                      0x00U
#define AUDIO_STREAMING_GENERAL                       0x01U
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02U

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24U
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25U

/* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01U
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02U
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03U
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06U

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x0CU
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x09U
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07U

#define AUDIO_CONTROL_MUTE                            0x0001U
#define AUDIO_CONTROL_VOLUME                          0x0002U

#define AUDIO_FORMAT_TYPE_I                           0x01U
#define AUDIO_FORMAT_TYPE_III                         0x03U

#define AUDIO_ENDPOINT_GENERAL                        0x01U

#define AUDIO_REQ_GET_CUR                             0x81U
#define AUDIO_REQ_SET_CUR                             0x01U

#define AUDIO_OUT_STREAMING_CTRL                      0x02U


#define AUDIO_OUT_PACKET                              (uint16_t)(((USBD_AUDIO_FREQ * 2U * 2U) / 1000U))
#define AUDIO_DEFAULT_VOLUME                          70U

/* Number of sub-packets in the audio transfer buffer. You can modify this value but always make sure
  that it is an even number and higher than 3 */
#define AUDIO_OUT_PACKET_NUM                          80U
  /* Total size of the audio transfer buffer */
#define AUDIO_TOTAL_BUF_SIZE                          ((uint16_t)(AUDIO_OUT_PACKET * AUDIO_OUT_PACKET_NUM))

	/* Audio Commands enumeration */
	typedef enum
	{
		AUDIO_CMD_START = 1,
		AUDIO_CMD_PLAY,
		AUDIO_CMD_STOP,
	}AUDIO_CMD_TypeDef;


	typedef enum
	{
		AUDIO_OFFSET_NONE = 0,
		AUDIO_OFFSET_HALF,
		AUDIO_OFFSET_FULL,
		AUDIO_OFFSET_UNKNOWN,
	}
	AUDIO_OffsetTypeDef;
	/**
	  * @}
	  */


	  /** @defgroup USBD_CORE_Exported_TypesDefinitions
		* @{
		*/
	typedef struct
	{
		uint8_t cmd;
		uint8_t data[USBD_EP0_BUFFER_SIZE];
		uint8_t len;
		uint8_t unit;
	}
	USBD_AUDIO_ControlTypeDef;



	typedef struct
	{
		uint32_t                  alt_setting;
		uint8_t                   buffer[AUDIO_TOTAL_BUF_SIZE];
		AUDIO_OffsetTypeDef       offset;
		uint8_t                    rd_enable;
		uint16_t                   rd_ptr;
		uint16_t                   wr_ptr;
		USBD_AUDIO_ControlTypeDef control;
	}
	USBD_AUDIO_HandleTypeDef;



	/** @brief AUDIO application structure */
	typedef struct
	{
		const char* Name;   /*!< String description of the application */
		int8_t(*Init)         (void* itf, uint32_t  AudioFreq, uint32_t Volume, uint32_t options);
		int8_t(*DeInit)       (void* itf, uint32_t options);
		int8_t(*AudioCmd)     (void* itf, uint8_t* pbuf, uint32_t size, uint8_t cmd);
		int8_t(*VolumeCtl)    (void* itf, uint8_t vol);
		int8_t(*MuteCtl)      (void* itf, uint8_t cmd);
		int8_t(*PeriodicTC)   (void* itf, uint8_t cmd);
		int8_t(*GetState)     (void* itf);
	}USBD_AUDIO_AppType;


	/** @brief AUDIO interface configuration */
	typedef struct
	{
		uint8_t Protocol;   /*!< Protocol used for Control requests */
		uint8_t OutEpNum;   /*!< OUT endpoint address */
		uint8_t InEpNum;   /*!< IN endpoint address */
		USBD_AUDIO_HandleTypeDef handle;
	}USBD_AUDIO_ConfigType;

	/** @brief AUDIO class interface structure */
	typedef struct
	{
		USBD_IfHandleType Base;             /*!< Class-independent interface base */
		const USBD_AUDIO_AppType* App;        /*!< AUDIO application reference */
		USBD_AUDIO_ConfigType Config;         /*!< AUDIO interface configuration */
		USBD_PADDING_1();
	}USBD_AUDIO_IfHandleType;

#define AUDIO_APP(ITF)    ((USBD_AUDIO_AppType*)((ITF)->App))

	/** @defgroup USBD_CORE_Exported_Macros
	  * @{
	  */

	  /**
		* @}
		*/

		/** @defgroup USBD_CORE_Exported_Variables
		  * @{
		  */

	extern USBD_ClassType USBD_AUDIO;
	extern USBD_ReturnType USBD_AUDIO_MountInterface(USBD_AUDIO_IfHandleType *itf, USBD_HandleType *dev);
extern void USBD_AUDIO_TEST(USBD_AUDIO_IfHandleType *pdev);	
#define USBD_AUDIO_CLASS    &USBD_AUDIO
	/**
	  * @}
	  */

	  /** @defgroup USB_CORE_Exported_Functions
		* @{
		*/
	uint8_t  USBD_AUDIO_RegisterInterface(USBD_AUDIO_IfHandleType   *pdev, USBD_AUDIO_AppType *fops);

	void  USBD_AUDIO_Sync(USBD_AUDIO_IfHandleType *pdev, AUDIO_OffsetTypeDef offset);
	/**
	  * @}
	  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_AUDIO_H */
/**
  * @}
  */

  /**
	* @}
	*/

	/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
