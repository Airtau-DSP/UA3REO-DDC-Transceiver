/**
  ******************************************************************************
  * @file    usbd_cdc.h
  * @author  MCD Application Team
  * @brief   header file for the usbd_cdc.c file.
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
#ifndef __USB_CDC_H
#define __USB_CDC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"
#include  "audio_processor.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup usbd_cdc
  * @brief This file is the Header file for usbd_cdc.c
  * @{
  */


/** @defgroup usbd_cdc_Exported_Defines
  * @{
  */

#define DEBUG_INTERFACE_IDX 0x0                            	// Index of DEBUG interface
#define CAT_INTERFACE_IDX 0x2                            	// Index of CAT interface
#define AUDIO_INTERFACE_IDX 0x3                            	// Index of AUDIO interface

//#define DEBUG_CMD_EP_IDX                  0x01
#define DEBUG_EP_IDX                      0x01
//#define CAT_CMD_EP_IDX                  0x03
#define CAT_EP_IDX                      0x02
#define AUDIO_EP_IDX                      0x03

#define IN_EP_DIR						0x80 // Adds a direction bit

//#define DEBUG_CMD_EP                      DEBUG_CMD_EP_IDX| IN_EP_DIR   
#define DEBUG_OUT_EP                      DEBUG_EP_IDX                  
#define DEBUG_IN_EP                       DEBUG_EP_IDX | IN_EP_DIR      
//#define CAT_CMD_EP                      CAT_CMD_EP_IDX| IN_EP_DIR   
#define CAT_OUT_EP                      CAT_EP_IDX                  
#define CAT_IN_EP                       CAT_EP_IDX | IN_EP_DIR      
#define AUDIO_OUT_EP                      AUDIO_EP_IDX                  
#define AUDIO_IN_EP                       AUDIO_EP_IDX | IN_EP_DIR     

#ifndef CDC_HS_BINTERVAL
  #define CDC_HS_BINTERVAL                          0x10U
#endif /* CDC_HS_BINTERVAL */

#ifndef CDC_FS_BINTERVAL
  #define CDC_FS_BINTERVAL                          0x10U
#endif /* CDC_FS_BINTERVAL */

/* CDC Endpoints parameters: you can fine tune these values depending on the needed baudrates and performance. */
#define CDC_DATA_HS_MAX_PACKET_SIZE                 64U  /* Endpoint IN & OUT Packet size */
#define CDC_DATA_FS_MAX_PACKET_SIZE                 64U  /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE                         8U  /* Control Endpoint Packet size */

//#define USB_CDC_CONFIG_DESC_SIZ                     127U
#define USB_CDC_CONFIG_DESC_SIZ                     226U

#define CDC_DATA_HS_IN_PACKET_SIZE                  CDC_DATA_HS_MAX_PACKET_SIZE
#define CDC_DATA_HS_OUT_PACKET_SIZE                 CDC_DATA_HS_MAX_PACKET_SIZE

#define CDC_DATA_FS_IN_PACKET_SIZE                  CDC_DATA_FS_MAX_PACKET_SIZE
#define CDC_DATA_FS_OUT_PACKET_SIZE                 CDC_DATA_FS_MAX_PACKET_SIZE

/*---------------------------------------------------------------------*/
/*  CDC definitions                                                    */
/*---------------------------------------------------------------------*/
#define CDC_SEND_ENCAPSULATED_COMMAND               0x00U
#define CDC_GET_ENCAPSULATED_RESPONSE               0x01U
#define CDC_SET_COMM_FEATURE                        0x02U
#define CDC_GET_COMM_FEATURE                        0x03U
#define CDC_CLEAR_COMM_FEATURE                      0x04U
#define CDC_SET_LINE_CODING                         0x20U
#define CDC_GET_LINE_CODING                         0x21U
#define CDC_SET_CONTROL_LINE_STATE                  0x22U
#define CDC_SEND_BREAK                              0x23U

//AUDIO
#define USBD_AUDIO_FREQ                               48000U
#define AUDIO_OUT_PACKET                              (uint16_t)(((USBD_AUDIO_FREQ * 2U * 2U) / 1000U))
//#define AUDIO_OUT_PACKET_NUM                          5U
//#define AUDIO_TOTAL_BUF_SIZE                          ((uint16_t)(AUDIO_OUT_PACKET * AUDIO_OUT_PACKET_NUM))
#define AUDIO_BUFFER_SIZE															FPGA_AUDIO_BUFFER_SIZE*2
#define AUDIO_DEFAULT_VOLUME                          70U

#define AUDIO_REQ_GET_CUR                             0x81U
#define AUDIO_REQ_SET_CUR                             0x01U
#define AUDIO_OUT_STREAMING_CTRL                      0x02U
#define USB_AUDIO_DESC_SIZ                            0x09U
#define AUDIO_DESCRIPTOR_TYPE                         0x21U

/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */

/**
  * @}
  */
typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
}USBD_CDC_LineCodingTypeDef;

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

typedef struct
{
   uint8_t cmd;
   uint8_t data[USB_MAX_EP0_SIZE];
   uint8_t len;
   uint8_t unit;
}
USBD_AUDIO_ControlTypeDef;

typedef struct _USBD_DEBUG_Itf
{
  int8_t (* Init)          (void);
  int8_t (* DeInit)        (void);
  int8_t (* Control)       (uint8_t cmd, uint8_t* pbuf, uint16_t length);
  int8_t (* Receive)       (uint8_t* Buf, uint32_t *Len);

}USBD_DEBUG_ItfTypeDef;

typedef struct _USBD_CAT_Itf
{
  int8_t (* Init)          (void);
  int8_t (* DeInit)        (void);
  int8_t (* Control)       (uint8_t cmd, uint8_t* pbuf, uint16_t length);
  int8_t (* Receive)       (uint8_t* Buf, uint32_t *Len);

}USBD_CAT_ItfTypeDef;

typedef struct
{
    int8_t  (*Init)         (uint32_t  AudioFreq, uint32_t Volume, uint32_t options);
    int8_t  (*DeInit)       (uint32_t options);
    int8_t  (*AudioCmd)     (uint8_t* pbuf, uint32_t size, uint8_t cmd);
    int8_t  (*VolumeCtl)    (uint8_t vol);
    int8_t  (*MuteCtl)      (uint8_t cmd);
    int8_t  (*PeriodicTC)   (uint8_t cmd);
    int8_t  (*GetState)     (void);
}USBD_AUDIO_ItfTypeDef;

typedef struct
{
  uint32_t                  alt_setting;
  //uint8_t                   buffer[AUDIO_TOTAL_BUF_SIZE];
  AUDIO_OffsetTypeDef       offset;
  uint8_t                    rd_enable;
  uint16_t                   rd_ptr;
  uint16_t                   wr_ptr;
  USBD_AUDIO_ControlTypeDef control;
	
	uint8_t  *TxBuffer;
	uint32_t TxLength;
}
USBD_AUDIO_HandleTypeDef;

typedef struct
{
  uint32_t data[CDC_DATA_HS_MAX_PACKET_SIZE / 4U];      /* Force 32bits alignment */
  uint8_t  CmdOpCode;
  uint8_t  CmdLength;
  uint8_t  *RxBuffer;
  uint8_t  *TxBuffer;
  uint32_t RxLength;
  uint32_t TxLength;

  __IO uint32_t TxState;
  __IO uint32_t RxState;
}
USBD_DEBUG_HandleTypeDef;

typedef struct
{
  uint32_t data[CDC_DATA_HS_MAX_PACKET_SIZE / 4U];      /* Force 32bits alignment */
  uint8_t  CmdOpCode;
  uint8_t  CmdLength;
  uint8_t  *RxBuffer;
  uint8_t  *TxBuffer;
  uint32_t RxLength;
  uint32_t TxLength;

  __IO uint32_t TxState;
  __IO uint32_t RxState;
}
USBD_CAT_HandleTypeDef;


/** @defgroup USBD_CORE_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CORE_Exported_Variables
  * @{
  */

extern USBD_ClassTypeDef  USBD_UA3REO;
#define USBD_UA3REO_CLASS    &USBD_UA3REO
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
uint8_t  USBD_DEBUG_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_DEBUG_ItfTypeDef *fops);
uint8_t  USBD_DEBUG_SetTxBuffer        (USBD_HandleTypeDef   *pdev, uint8_t  *pbuff, uint16_t length);
uint8_t  USBD_DEBUG_SetRxBuffer        (USBD_HandleTypeDef   *pdev,  uint8_t  *pbuff);
uint8_t  USBD_DEBUG_ReceivePacket      (USBD_HandleTypeDef *pdev);
uint8_t  USBD_DEBUG_TransmitPacket     (USBD_HandleTypeDef *pdev);

uint8_t  USBD_CAT_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_CAT_ItfTypeDef *fops);
uint8_t  USBD_CAT_SetTxBuffer        (USBD_HandleTypeDef   *pdev, uint8_t  *pbuff, uint16_t length);
uint8_t  USBD_CAT_SetRxBuffer        (USBD_HandleTypeDef   *pdev,  uint8_t  *pbuff);
uint8_t  USBD_CAT_ReceivePacket      (USBD_HandleTypeDef *pdev);
uint8_t  USBD_CAT_TransmitPacket     (USBD_HandleTypeDef *pdev);

uint8_t  USBD_AUDIO_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_AUDIO_ItfTypeDef *fops);
uint8_t  USBD_AUDIO_StartTransmit (USBD_HandleTypeDef *pdev);
uint8_t  USBD_AUDIO_SetTxBuffer  (USBD_HandleTypeDef *pdev,uint8_t  *pbuff, uint16_t length);
void  USBD_AUDIO_Sync (USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_CDC_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
