/**
  ******************************************************************************
  * @file    usbd_audio.c
  * @author  MCD Application Team
  * @brief   This file provides the Audio core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                AUDIO Class  Description
  *          ===================================================================
 *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
  *           Audio Devices V1.0 Mar 18, 98".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Standard AC Interface Descriptor management
  *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
  *             - 1 Audio Streaming Endpoint
  *             - 1 Audio Terminal Input (1 channel)
  *             - Audio Class-Specific AC Interfaces
  *             - Audio Class-Specific AS Interfaces
  *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
  *             - Audio Feature Unit (limited to Mute control)
  *             - Audio Synchronization type: Asynchronous
  *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
  *          The current audio class version supports the following audio features:
  *             - Pulse Coded Modulation (PCM) format
  *             - sampling rate: 48KHz.
  *             - Bit resolution: 16
  *             - Number of channels: 2
  *             - No volume control
  *             - Mute/Unmute capability
  *             - Asynchronous Endpoints
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
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

  /* BSPDependencies
  - "stm32xxxxx_{eval}{discovery}.c"
  - "stm32xxxxx_{eval}{discovery}_io.c"
  - "stm32xxxxx_{eval}{discovery}_audio.c"
  EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usbd_types.h"
#include <stdlib.h>
#include <private/usbd_internal.h>
//#include "usbd_ctlreq.h"

/** @defgroup USBD_AUDIO_Private_Macros
  * @{
  */
#define AUDIO_SAMPLE_FREQ(frq)      (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2U * 2U)/1000U) & 0xFFU), \


/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
  * @{
  */

static uint8_t  USBD_AUDIO_Init (USBD_AUDIO_IfHandleType *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_DeInit (USBD_AUDIO_IfHandleType *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_Setup (USBD_AUDIO_IfHandleType *pdev);
static uint16_t audio_getDesc(USBD_AUDIO_IfHandleType *itf, uint8_t ifNum, uint8_t * dest);
static uint8_t  USBD_AUDIO_DataIn (USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_DataOut (USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_EP0_RxReady (USBD_AUDIO_IfHandleType *pdev);
static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_AUDIO_IfHandleType *pdev);
static uint8_t  USBD_AUDIO_SOF (USBD_AUDIO_IfHandleType *pdev);
static uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static void AUDIO_REQ_GetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req);
static void AUDIO_REQ_SetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req);
static void audio_dataStage(USBD_AUDIO_IfHandleType *itf);
static const char* audio_getString(USBD_AUDIO_IfHandleType *itf, uint8_t intNum);

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Variables
  * @{
  */

static const USBD_ClassType audio_cbks = {
    .GetDescriptor  = (USBD_IfDescCbkType)  audio_getDesc,
    .GetString      = (USBD_IfStrCbkType)   audio_getString,
    .Deinit         = (USBD_IfCbkType)      USBD_AUDIO_DeInit,
    .SetupStage     = (USBD_IfSetupCbkType) USBD_AUDIO_Setup,
    .DataStage      = (USBD_IfCbkType)      audio_dataStage,
    .OutData        = (USBD_IfEpCbkType)    USBD_AUDIO_DataOut,
    .InData         = (USBD_IfEpCbkType)    USBD_AUDIO_DataIn,
};

typedef struct
{
    /* Interface Association Descriptor */
    USB_IfAssocDescType IAD;
    /* Communication Interface Descriptor */
    USB_InterfaceDescType ADID;
    /* Header Functional Descriptor */
    struct {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint16_t bcdCDC;
				uint16_t wTotalLength;
				uint8_t bInCollection;
				uint8_t baInterfaceNr;
    }__packed ADFD;
    /* USB Speaker Input Terminal Descriptor */
		struct {
				uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bTerminalID;
				uint16_t wTerminalType;
				uint8_t bAssocTerminal;
				uint8_t bNrChannels;
				uint16_t wChannelConfig;
				uint8_t iChannelNames;
				uint8_t iTerminal;
    }__packed ITD;
		/* USB Speaker Audio Feature Unit Descriptor */
		struct {
				uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bUnitID;
				uint8_t bSourceID;
				uint8_t bControlSize;
				uint8_t bmaControls0;
				uint8_t bmaControls1;
				uint8_t iTerminal;
    }__packed FUD;
		/* USB Speaker Output Terminal Descriptor */
		struct {
				uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bTerminalID;
				uint16_t wTerminalType;
				uint8_t bAssocTerminal;
				uint8_t bSourceID;
				uint8_t iTerminal;
    }__packed OTD;
    /* USB Speaker Audio Streaming Interface Descriptor */
		struct {
				uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bTerminalLink;
				uint8_t bDelay;
				uint16_t wFormatTag;
    }__packed ASID;
		/* USB Speaker Audio Type III Format Interface Descriptor */
		struct {
				uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bNrChannels;
				uint8_t bSubFrameSize;
				uint8_t bBitResolution;
				uint8_t bSamFreqType;
				uint8_t afreq1;
				uint8_t afreq2;
				uint8_t afreq3;
    }__packed ASFID;
		
    /* Endpoint descriptors are dynamically added */
}__packed USBD_AUDIO_DescType;

/* USB AUDIO device Configuration Descriptor */
static const USBD_AUDIO_DescType audio_desc = {
	.IAD = { /* Interface Association Descriptor */
        .bLength            = sizeof(audio_desc.IAD),
        .bDescriptorType    = USB_DESC_TYPE_IAD,
        .bFirstInterface    = 0,
        .bInterfaceCount    = 2,
        .bFunctionClass     = USB_DEVICE_CLASS_AUDIO, /* bFunctionClass: Communication Interface Class */
        .bFunctionSubClass  = AUDIO_SUBCLASS_AUDIOCONTROL, /* bFunctionSubClass: Abstract Control Model */
        .bFunctionProtocol  = AUDIO_PROTOCOL_UNDEFINED, /* bFunctionProtocol: Common AT commands */
        .iFunction          = USBD_ISTR_INTERFACES,
    },
	.ADID = { /* USB Speaker Class-specific AC Interface Descriptor */
        .bLength            = sizeof(audio_desc.ADID),
        .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
        .bInterfaceNumber   = 1,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 1,
        .bInterfaceClass    = USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass: Communication Interface Class */
        .bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass: Abstract Control Model */
        .bInterfaceProtocol = AUDIO_PROTOCOL_UNDEFINED, /* bInterfaceProtocol: Common AT commands */
        .iInterface         = USBD_ISTR_INTERFACES,
    },
  .ADFD = { /* Header Functional Descriptor */
        .bLength            = sizeof(audio_desc.ADFD),
        .bDescriptorType    = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
        .bDescriptorSubtype = AUDIO_CONTROL_HEADER, /* bDescriptorSubtype: Header Func Desc */
        .bcdCDC             = 0x100,/* bcdCDC: spec release number v1.00 */
				.wTotalLength       = 0x27,/* wTotalLength = 39 */
				.bInCollection             = 0x01,/* bInCollection */
				.baInterfaceNr             = 0x01,/* baInterfaceNr */
    },
	.ITD = { /* USB Speaker Input Terminal Descriptor */
        .bLength            = sizeof(audio_desc.ITD),
        .bDescriptorType    = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
        .bDescriptorSubtype = AUDIO_CONTROL_HEADER, /* bDescriptorSubtype: Header Func Desc */
        .bTerminalID             = 0x01,
				.wTerminalType       = 0x0101,
				.bAssocTerminal             = 0x00,
				.bNrChannels             = 0x01,
				.wChannelConfig				= 0x0000,
				.iChannelNames				= 0x00,
				.iTerminal						= 0x00,
    },
	.FUD = { /* USB Speaker Audio Feature Unit Descriptor */
        .bLength            = sizeof(audio_desc.FUD),
        .bDescriptorType    = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
        .bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT, /* bDescriptorSubtype: Header Func Desc */
        .bUnitID             = AUDIO_OUT_STREAMING_CTRL,
				.bSourceID       = 0x01,
				.bControlSize             = 0x01,
				.bmaControls0             = AUDIO_CONTROL_MUTE,
				.bmaControls1				= 0x0,
				.iTerminal						= 0x00,
    },
	.OTD = { /* USB Speaker Output Terminal Descriptor */
        .bLength            = sizeof(audio_desc.OTD),
        .bDescriptorType    = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
        .bDescriptorSubtype = AUDIO_CONTROL_OUTPUT_TERMINAL, /* bDescriptorSubtype: Header Func Desc */
        .bTerminalID             = 0x03,
				.wTerminalType       = 0x0301,
				.bAssocTerminal             = 0x00,
				.bSourceID             = 0x02,
				.iTerminal						= 0x00,
    },
	.ASID = { /* USB Speaker Audio Streaming Interface Descriptor */
        .bLength            = sizeof(audio_desc.ASID),
        .bDescriptorType    = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
        .bDescriptorSubtype = AUDIO_STREAMING_GENERAL, /* bDescriptorSubtype: Header Func Desc */
        .bTerminalLink             = 0x01,
				.bDelay       = 0x01,
				.wFormatTag             = 0x0001,
    },
	.ASFID = { /* USB Speaker Audio Type III Format Interface Descriptor */
        .bLength            = sizeof(audio_desc.ASID),
        .bDescriptorType    = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
        .bDescriptorSubtype = AUDIO_FORMAT_TYPE_I, /* bDescriptorSubtype: Header Func Desc */
        .bNrChannels             = 0x02,
				.bSubFrameSize       = 0x02,
				.bBitResolution             = 16,
				.bSamFreqType             = 0x01,
				.afreq1             = (uint8_t)(USBD_AUDIO_FREQ),
				.afreq2             = (uint8_t)((USBD_AUDIO_FREQ >> 8)),
				.afreq3             = (uint8_t)((USBD_AUDIO_FREQ >> 16)),
    },
		
  /* Endpoint 1 - Standard Descriptor */
  //AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  //USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  //AUDIO_OUT_EP,                         /* bEndpointAddress 1 out endpoint*/
  //USB_EP_TYPE_ISOCHRONOUS,                    /* bmAttributes */
  //AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),    /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  //0x01,                                 /* bInterval */
  //0x00,                                 /* bRefresh */
  //0x00,                                 /* bSynchAddress */
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor*/
  //AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  //AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  //AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  //0x00,                                 /* bmAttributes */
  //0x00,                                 /* bLockDelayUnits */
  //0x00,                                 /* wLockDelay */
  //0x00,
  /* 07 byte*/
} ;


/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Functions
  * @{
  */

/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_Init (USBD_AUDIO_IfHandleType *pdev, uint8_t cfgidx)
{
	USBD_HandleType *dev = pdev->Base.Device;
  USBD_AUDIO_HandleTypeDef   *haudio;

  /* Open EP OUT */
  USBD_EpOpen(dev, pdev->Config.OutEpNum, USB_EP_TYPE_ISOCHRONOUS, AUDIO_OUT_PACKET);

  /* Allocate Audio structure */
  haudio = (USBD_AUDIO_HandleTypeDef*) &pdev->Config.handle;
  haudio->alt_setting = 0U;
  haudio->offset = AUDIO_OFFSET_UNKNOWN;
  haudio->wr_ptr = 0U;
  haudio->rd_ptr = 0U;
  haudio->rd_enable = 0U;

  /* Initialize the Audio output Hardware layer */
  USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->Init, pdev,USBD_AUDIO_FREQ,AUDIO_DEFAULT_VOLUME,0U);

  /* Prepare Out endpoint to receive 1st packet */
	USBD_EpReceive(pdev->Base.Device, pdev->Config.OutEpNum, haudio->buffer, AUDIO_OUT_PACKET);
  return USBD_E_OK;
}

/**
  * @brief  USBD_AUDIO_Init
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DeInit (USBD_AUDIO_IfHandleType *pdev,
                                 uint8_t cfgidx)
{
	USBD_HandleType *dev = pdev->Base.Device;
	USBD_EpClose(dev, pdev->Config.OutEpNum);
  USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->DeInit, pdev,0U);

  return USBD_E_OK;
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_AUDIO_Setup (USBD_AUDIO_IfHandleType *pdev)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  USBD_ReturnType retval = USBD_E_INVALID;
  USBD_HandleType *dev = pdev->Base.Device;
	USB_SetupRequestType req=dev->Setup;

  switch (req.RequestType.Type)
  {
		case USB_REQ_TYPE_CLASS :
			switch (req.Request)
			{
			case AUDIO_REQ_GET_CUR:
				AUDIO_REQ_GetCurrent(pdev, &req);
				break;

			case AUDIO_REQ_SET_CUR:
				AUDIO_REQ_SetCurrent(pdev, &req);
				break;

			default:
				retval = USBD_E_INVALID;
				break;
			}
			break;

		default:
			retval = USBD_E_INVALID;
			break;
  }

  return retval;
}


static uint16_t audio_getDesc(USBD_AUDIO_IfHandleType *itf, uint8_t ifNum, uint8_t * dest)
{
    USBD_AUDIO_DescType *desc = (USBD_AUDIO_DescType*)dest;
    uint16_t len = sizeof(audio_desc);

    memcpy(dest, &audio_desc, sizeof(audio_desc));

    /* Adjustment of interface indexes */
    desc->IAD.bFirstInterface  = ifNum;
    desc->IAD.iFunction  = USBD_IIF_INDEX(ifNum, 0);

    desc->ADID.bInterfaceNumber = ifNum;

		//desc->ADFD.iInterface = USBD_IIF_INDEX(ifNum, 0);

    if (itf->Config.Protocol != 0)
        desc->IAD.bFunctionProtocol  = itf->Config.Protocol;

    len += USBD_EpDesc(itf->Base.Device, itf->Config.OutEpNum, &dest[len]);

    return len;
}

static const char* audio_getString(USBD_AUDIO_IfHandleType *itf, uint8_t intNum)
{
    return itf->App->Name;
}

static void audio_dataStage(USBD_AUDIO_IfHandleType *itf)
{
    USBD_HandleType *dev = itf->Base.Device;
	/*
    if (dev->Setup.Request == CDC_REQ_SET_LINE_CODING)
    {
			USBD_AUDIO_Init(itf,0);
    }
	*/
}

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DataIn (USBD_AUDIO_IfHandleType *pdev,
                              uint8_t epnum)
{

  /* Only OUT data are processed */
  return USBD_E_OK;
}

/**
  * @brief  USBD_AUDIO_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_EP0_RxReady (USBD_AUDIO_IfHandleType *pdev)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) &pdev->Config.handle;

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR)
  {/* In this driver, to simplify code, only SET_CUR request is managed */

    if (haudio->control.unit == AUDIO_OUT_STREAMING_CTRL)
    {
			USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->MuteCtl, pdev, haudio->control.data[0]);
      haudio->control.cmd = 0U;
      haudio->control.len = 0U;
    }
  }

  return USBD_E_OK;
}
/**
  * @brief  USBD_AUDIO_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_AUDIO_IfHandleType *pdev)
{
  /* Only OUT control data are processed */
  return USBD_E_OK;
}
/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_SOF (USBD_AUDIO_IfHandleType *pdev)
{
  return USBD_E_OK;
}

/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
void  USBD_AUDIO_Sync (USBD_AUDIO_IfHandleType *pdev, AUDIO_OffsetTypeDef offset)
{
  uint32_t cmd = 0U;
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) &pdev->Config.handle;

  haudio->offset =  offset;

  if(haudio->rd_enable == 1U)
  {
    haudio->rd_ptr += (uint16_t)(AUDIO_TOTAL_BUF_SIZE / 2U);

    if (haudio->rd_ptr == AUDIO_TOTAL_BUF_SIZE)
    {
      /* roll back */
      haudio->rd_ptr = 0U;
    }
  }

  if(haudio->rd_ptr > haudio->wr_ptr)
  {
    if((haudio->rd_ptr - haudio->wr_ptr) < AUDIO_OUT_PACKET)
    {
      cmd = AUDIO_TOTAL_BUF_SIZE / 2U + 4U;
    }
    else
    {
      if((haudio->rd_ptr - haudio->wr_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
      {
        cmd = AUDIO_TOTAL_BUF_SIZE / 2U - 4U;
      }
    }
  }
  else
  {
    if((haudio->wr_ptr - haudio->rd_ptr) < AUDIO_OUT_PACKET)
    {
      cmd = AUDIO_TOTAL_BUF_SIZE / 2U - 4U;
    }
    else
    {
      if((haudio->wr_ptr - haudio->rd_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
      {
        cmd = AUDIO_TOTAL_BUF_SIZE / 2U + 4U;
      }
    }
  }

  if(haudio->offset == AUDIO_OFFSET_FULL)
  {
		USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->AudioCmd, pdev, &haudio->buffer[0],cmd,AUDIO_CMD_PLAY);
    haudio->offset = AUDIO_OFFSET_NONE;
  }
}

/**
  * @brief  USBD_AUDIO_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DataOut (USBD_AUDIO_IfHandleType *pdev,
                              uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) &pdev->Config.handle;

  if (epnum == pdev->Config.OutEpNum)
  {
    /* Increment the Buffer pointer or roll it back when all buffers are full */

    haudio->wr_ptr += AUDIO_OUT_PACKET;

    if (haudio->wr_ptr == AUDIO_TOTAL_BUF_SIZE)
    {
      /* All buffers are full: roll back */
      haudio->wr_ptr = 0U;

      if(haudio->offset == AUDIO_OFFSET_UNKNOWN)
      {
				USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->AudioCmd, pdev, &haudio->buffer[0],AUDIO_TOTAL_BUF_SIZE / 2U,AUDIO_CMD_START);
        haudio->offset = AUDIO_OFFSET_NONE;
      }
    }

    if(haudio->rd_enable == 0U)
    {
      if (haudio->wr_ptr == (AUDIO_TOTAL_BUF_SIZE / 2U))
      {
        haudio->rd_enable = 1U;
      }
    }

    /* Prepare Out endpoint to receive next audio packet */
		USBD_EpReceive(pdev->Base.Device, pdev->Config.OutEpNum, &haudio->buffer[haudio->wr_ptr], AUDIO_OUT_PACKET);
  }

  return USBD_E_OK;
}

/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) &pdev->Config.handle;

  memset(haudio->control.data, 0, 64U);

  /* Send the current mute state */
	USBD_EpSend(pdev->Base.Device, pdev->Config.OutEpNum, haudio->control.data, req->Length);
}

/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_SetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) &pdev->Config.handle;

  if (req->Length)
  {
    /* Prepare the reception of the buffer over EP0 */
		USBD_EpReceive(pdev->Base.Device, pdev->Config.OutEpNum, haudio->control.data, req->Length);

    haudio->control.cmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
    haudio->control.len = (uint8_t)req->Length; /* Set the request data length */
    haudio->control.unit = HIBYTE(req->Index);  /* Set the request target unit */
  }
}

/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t  USBD_AUDIO_RegisterInterface  (USBD_AUDIO_IfHandleType   *pdev, USBD_AUDIO_AppType *fops)
{
  if(fops != NULL)
  {
		pdev->App=fops;
  }
  return USBD_E_OK;
}

USBD_ReturnType USBD_AUDIO_MountInterface(USBD_AUDIO_IfHandleType *itf, USBD_HandleType *dev)
{
    USBD_ReturnType retval = USBD_E_ERROR;

    /* Note: CDC uses 2 interfaces */
    if (dev->IfCount < (USBD_MAX_IF_COUNT - 1))
    {
        /* Binding interfaces */
        itf->Base.Device = dev;
        itf->Base.Class  = &audio_cbks;
        itf->Base.AltCount = 1;
        itf->Base.AltSelector = 0;

        {
            USBD_EpHandleType *ep;

            ep = USBD_EpAddr2Ref(dev, itf->Config.OutEpNum);
            ep->Type            = USB_EP_TYPE_ISOCHRONOUS;
            ep->MaxPacketSize   = AUDIO_OUT_PACKET;
            ep->IfNum           = dev->IfCount;
        }

        dev->IF[dev->IfCount] = (USBD_IfHandleType*)itf;
        dev->IfCount++;

        dev->IF[dev->IfCount] = (USBD_IfHandleType*)itf;
        dev->IfCount++;

        retval = USBD_E_OK;
    }

    return retval;
}

