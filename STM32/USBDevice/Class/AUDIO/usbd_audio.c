/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usb_types.h"
#include "usbd_types.h"
#include <stdlib.h>
#include <private/usbd_internal.h>
#include "../../Src/functions.h"

/** @defgroup USBD_AUDIO_Private_Macros
  * @{
  */
#define AUDIO_SAMPLE_FREQ(frq)      (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2U * 2U)/1000U) & 0xFFU) // USB_EP_ISOC_FS_MPS ?

  /** @defgroup USBD_AUDIO_Private_FunctionPrototypes
	* @{
	*/

static void  USBD_AUDIO_Init(USBD_AUDIO_IfHandleType *pdev);
static void  USBD_AUDIO_DeInit(USBD_AUDIO_IfHandleType *pdev);
static uint8_t  USBD_AUDIO_Setup(USBD_AUDIO_IfHandleType *pdev);
static uint16_t USBD_AUDIO_GetDesc(USBD_AUDIO_IfHandleType *itf, uint8_t ifNum, uint8_t * dest);
static uint8_t  USBD_AUDIO_DataIn(USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_DataOut(USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_EP0_RxReady(USBD_AUDIO_IfHandleType *pdev);
static uint8_t  USBD_AUDIO_EP0_TxReady(USBD_AUDIO_IfHandleType *pdev);
static uint8_t  USBD_AUDIO_SOF(USBD_AUDIO_IfHandleType *pdev);
static uint8_t  USBD_AUDIO_IsoINIncomplete(USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_IsoOutIncomplete(USBD_AUDIO_IfHandleType *pdev, uint8_t epnum);
static USBD_ReturnType AUDIO_REQ_GetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req);
static USBD_ReturnType AUDIO_REQ_SetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req);
static void USBD_AUDIO_DataStage(USBD_AUDIO_IfHandleType *itf);
static const char* USBD_AUDIO_GetString(USBD_AUDIO_IfHandleType *itf, uint8_t intNum);

/**
  * @}
  */

  /** @defgroup USBD_AUDIO_Private_Variables
	* @{
	*/

static const USBD_ClassType audio_cbks = {
	.GetDescriptor = (USBD_IfDescCbkType)USBD_AUDIO_GetDesc,
	.GetString = (USBD_IfStrCbkType)USBD_AUDIO_GetString,
	.Init = (USBD_IfCbkType)USBD_AUDIO_Init,
	.Deinit = (USBD_IfCbkType)USBD_AUDIO_DeInit,
	.SetupStage = (USBD_IfSetupCbkType)USBD_AUDIO_Setup,
	.DataStage = (USBD_IfCbkType)USBD_AUDIO_DataStage,
	.OutData = (USBD_IfEpCbkType)USBD_AUDIO_DataOut,
	.InData = (USBD_IfEpCbkType)USBD_AUDIO_DataIn,
};

typedef struct
{
	USB_IfAssocDescType IADA;
	/* Interface 0, Alternate Setting 0, Audio Control */
	USB_InterfaceDescType ACIF;
	/* Audio Control Interface */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bcdCDC1;
		uint8_t bcdCDC2;
		uint8_t wTotalLengthLSB;
		uint8_t wTotalLengthMSB;
		uint8_t bInCollection;
		uint8_t baInterfaceNr;
		uint8_t baInterfaceNr2;
	}__packed ACI;
	/* Audio Input Terminal (Speaker) */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bTerminalID;
		uint8_t wTerminalType1;
		uint8_t wTerminalType2;
		uint8_t bAssocTerminal;
		uint8_t bNrChannels;
		uint16_t wChannelConfig;
		uint8_t iChannelNames;
		uint8_t iTerminal;
	}__packed AITS;
	/* Audio Feature Unit (Speaker) */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bUnitID;
		uint8_t bSourceID;
		uint8_t bControlSize;
		uint8_t bmaControls0;
		uint8_t bmaControls1;
		uint8_t iFeature;
	}__packed AFUS;
	/* Audio Output Terminal (Speaker) */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bTerminalID;
		uint8_t wTerminalType1;
		uint8_t wTerminalType2;
		uint8_t bAssocTerminal;
		uint8_t bSourceID;
		uint8_t iTerminal;
	}__packed AOTS;
	/* Audio Input Terminal (Microphone) */
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
	}__packed AITM;
	/* Audio Output Terminal (Microphone) */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bTerminalID;
		uint16_t wTerminalType;
		uint8_t bAssocTerminal;
		uint8_t bSourceID;
		uint8_t iTerminal;
	}__packed AOTM;
	USB_InterfaceDescType ADID0; /* USB Speaker Audio Streaming Interface Descriptor alt0 */
	USB_InterfaceDescType ADID1; /* USB Speaker Audio Streaming Interface Descriptor alt1 */
/* USB Speaker Audio Streaming Interface Descriptor */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bTerminalLink;
		uint8_t bDelay;
		uint8_t wFormatTagLSB;
		uint8_t wFormatTagMSB;
	}__packed ASID;
	/* USB Speaker Audio Type I Format Interface Descriptor */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bFormatType;
		uint8_t bNrChannels;
		uint8_t bSubFrameSize;
		uint8_t bBitResolution;
		uint8_t bSamFreqType;
		uint8_t afreq1;
		uint8_t afreq2;
		uint8_t afreq3;
	}__packed ASFID;
	struct { /* USB Speaker EP1 */
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bEndpointAddress;
		uint8_t bmAttributes;
		uint16_t wMaxPacketSize;
		uint8_t bInterval;
		uint8_t bRefresh;
		uint8_t bSynchAddress;
	}__packed EP1;
	struct { /* USB Audio Data Endpoint Descriptor */
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubType;
		uint8_t bmAttributes;
		uint8_t bLockDelayUnits;
		uint8_t wLockDelay;
		uint8_t tmp;
	}__packed EP1D;
	USB_InterfaceDescType ADID02; /* USB Speaker Audio Streaming Interface Descriptor alt0 */
	USB_InterfaceDescType ADID12; /* USB Speaker Audio Streaming Interface Descriptor alt1 */
/* USB Speaker Audio Streaming Interface Descriptor */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bTerminalLink;
		uint8_t bDelay;
		uint8_t wFormatTagLSB;
		uint8_t wFormatTagMSB;
	}__packed ASID2;
	/* USB Speaker Audio Type I Format Interface Descriptor */
	struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubtype;
		uint8_t bFormatType;
		uint8_t bNrChannels;
		uint8_t bSubFrameSize;
		uint8_t bBitResolution;
		uint8_t bSamFreqType;
		uint8_t afreq1;
		uint8_t afreq2;
		uint8_t afreq3;
	}__packed ASFID2;
	struct { /* USB Speaker EP1 */
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bEndpointAddress;
		uint8_t bmAttributes;
		uint16_t wMaxPacketSize;
		uint8_t bInterval;
		uint8_t bRefresh;
		uint8_t bSynchAddress;
	}__packed EP12;
	struct { /* USB Audio Data Endpoint Descriptor */
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bDescriptorSubType;
		uint8_t bmAttributes;
		uint8_t bLockDelayUnits;
		uint8_t wLockDelay;
		uint8_t tmp;
	}__packed EP1D2;
}__packed USBD_AUDIO_DescType;

#define TOTAL_CONTROL_INTF_LENGTH    (sizeof(audio_desc.ACI) + 2*sizeof(audio_desc.AITS) + sizeof(audio_desc.AFUS) + 2*sizeof(audio_desc.AOTS))

/* USB AUDIO device Configuration Descriptor */
static const USBD_AUDIO_DescType audio_desc = {
		.IADA = {
		.bLength = sizeof(audio_desc.IADA),
		.bDescriptorType = USB_DESC_TYPE_IAD,
		.bFirstInterface = 0,
		.bInterfaceCount = 3,
		.bFunctionClass = USB_DEVICE_CLASS_AUDIO,
		.bFunctionSubClass = 0x0,
		.bFunctionProtocol = 0x0,
		.iFunction = 0x0,
	},
	.ACIF = { /* Interface 0, Alternate Setting 0, Audio Control */
		.bLength = sizeof(audio_desc.ACIF),
		.bDescriptorType = USB_DESC_TYPE_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_DEVICE_CLASS_AUDIO,
		.bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOCONTROL,
		.bInterfaceProtocol = 0x0,
		.iInterface = 0x0,
	},
  .ACI = { /* Audio Control Interface */
		.bLength = sizeof(audio_desc.ACI),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_CONTROL_HEADER, /* bDescriptorSubtype: Header Func Desc */
		.bcdCDC1 = 0x00,/* bcdCDC: spec release number v1.00 */
		.bcdCDC2 = 0x01,/* bcdCDC: spec release number v1.00 */
				.wTotalLengthLSB = (uint8_t)(TOTAL_CONTROL_INTF_LENGTH),
				.wTotalLengthMSB = (uint8_t)(TOTAL_CONTROL_INTF_LENGTH >> 8),
				.bInCollection = 0x02,/* bInCollection */
				.baInterfaceNr = 0x01,/* baInterfaceNr */
				.baInterfaceNr2 = 0x02,/* baInterfaceNr */
	},
	.AITS = { /* Audio Input Terminal (Speaker) */
		.bLength = sizeof(audio_desc.AITS),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_CONTROL_INPUT_TERMINAL, /* bDescriptorSubtype: Header Func Desc */
		.bTerminalID = 0x01,
				.wTerminalType1 = 0x01,
				.wTerminalType2 = 0x01,
				.bAssocTerminal = 0x00,
				.bNrChannels = 0x01,
				.wChannelConfig = 0x0000,
				.iChannelNames = 0x00,
				.iTerminal = 0x00,
	},
	.AFUS = { /* Audio Feature Unit (Speaker) */
		.bLength = sizeof(audio_desc.AFUS),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT, /* bDescriptorSubtype: Header Func Desc */
		.bUnitID = AUDIO_OUT_STREAMING_CTRL, //0x02
		.bSourceID = 0x01,
		.bControlSize = 0x01,
		.bmaControls0 = AUDIO_CONTROL_MUTE, // | AUDIO_CONTROL_VOLUME
		.bmaControls1 = 0x0,
		.iFeature = 0x00,
	},
	.AOTS = { /* Audio Output Terminal (Speaker) */
		.bLength = sizeof(audio_desc.AOTS),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_CONTROL_OUTPUT_TERMINAL, /* bDescriptorSubtype: Header Func Desc */
		.bTerminalID = 0x03,
				.wTerminalType1 = 0x01,
				.wTerminalType2 = 0x03,
				.bAssocTerminal = 0x00,
				.bSourceID = 0x02,
				.iTerminal = 0x00,
	},
	.AITM = { /* Audio Input Terminal (Microphone) */
		.bLength = sizeof(audio_desc.AITM),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_CONTROL_INPUT_TERMINAL, /* bDescriptorSubtype: Header Func Desc */
		.bTerminalID = 0x04,
				.wTerminalType = 0x0201,
				.bAssocTerminal = 0x00,
				.bNrChannels = 0x02,
				.wChannelConfig = 0x0000,
				.iChannelNames = 0x00,
				.iTerminal = 0x00,
	},
	.AOTM = { /* Audio Output Terminal (Microphone) */
		.bLength = sizeof(audio_desc.AOTM),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_CONTROL_OUTPUT_TERMINAL, /* bDescriptorSubtype: Header Func Desc */
		.bTerminalID = 0x05,
				.wTerminalType = 0x0101,
				.bAssocTerminal = 0x00,
				.bSourceID = 0x04,
				.iTerminal = 0x00,
	},
	.ADID0 = { /* Interface 1, Alternate Setting 0, Audio Streaming - Zero Bandwith */
		.bLength = sizeof(audio_desc.ADID0),
		.bDescriptorType = USB_DESC_TYPE_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass: Communication Interface Class */
		.bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass: Abstract Control Model */
		.bInterfaceProtocol = 0, /* bInterfaceProtocol: Common AT commands */
		.iInterface = 0,
	},
	.ADID1 = { /* Interface 1, Alternate Setting 1, Audio Streaming - Operational */
		.bLength = sizeof(audio_desc.ADID1),
		.bDescriptorType = USB_DESC_TYPE_INTERFACE,
		.bInterfaceNumber = 0x01,
		.bAlternateSetting = 0x01,
		.bNumEndpoints = 0x01,
		.bInterfaceClass = USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass: Communication Interface Class */
		.bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass: Abstract Control Model */
		.bInterfaceProtocol = 0, /* bInterfaceProtocol: Common AT commands */
		.iInterface = 0,
	},
	.ASID = { /* Audio Streaming Interface */
		.bLength = sizeof(audio_desc.ASID),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_STREAMING_GENERAL, /* bDescriptorSubtype: Header Func Desc */
		.bTerminalLink = 0x01,
		.bDelay = 0x00,
		.wFormatTagLSB = 0x01,
		.wFormatTagMSB = 0x00,
	},
	.ASFID = { /* Audio Type I Format */
		.bLength = sizeof(audio_desc.ASFID),
		.bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		.bDescriptorSubtype = AUDIO_STREAMING_FORMAT_TYPE,
				.bFormatType = AUDIO_FORMAT_TYPE_I,
		.bNrChannels = 0x02,
				.bSubFrameSize = 0x02,
				.bBitResolution = 0x10, //16 bit
				.bSamFreqType = 0x01,
				.afreq1 = (uint8_t)(USBD_AUDIO_FREQ),
				.afreq2 = (uint8_t)((USBD_AUDIO_FREQ >> 8)),
				.afreq3 = (uint8_t)((USBD_AUDIO_FREQ >> 16)),
	},

	/* Endpoint 1 - Standard Descriptor */
		  .EP1 = {
		  .bLength = sizeof(audio_desc.EP1),
		  .bDescriptorType = USB_DESC_TYPE_ENDPOINT, /* bDescriptorType: CS_INTERFACE */
		  .bEndpointAddress = 0x85,
				  .bmAttributes = USB_EP_TYPE_ISOCHRONOUS,
		  .wMaxPacketSize = AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),
				  .bInterval = 0x01,
				  .bRefresh = 0x00,
				  .bSynchAddress = 0x00,
	  },
		  .EP1D = { /* USB Audio Data Endpoint Descriptor */
		  .bLength = sizeof(audio_desc.EP1D),
		  .bDescriptorType = AUDIO_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		  .bDescriptorSubType = AUDIO_ENDPOINT_GENERAL,
				  .bmAttributes = 0x00,
		  .bLockDelayUnits = 0x00,
				  .wLockDelay = 0x00,
			.tmp = 0,
	  },
			
		.ADID02 = { /* Interface 1, Alternate Setting 0, Audio Streaming - Zero Bandwith */
		  .bLength = sizeof(audio_desc.ADID02),
		  .bDescriptorType = USB_DESC_TYPE_INTERFACE,
		  .bInterfaceNumber = 2,
		  .bAlternateSetting = 0,
		  .bNumEndpoints = 0,
		  .bInterfaceClass = USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass: Communication Interface Class */
		  .bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass: Abstract Control Model */
		  .bInterfaceProtocol = 0, /* bInterfaceProtocol: Common AT commands */
				  .iInterface = 0,
	  },
		  .ADID12 = { /* Interface 1, Alternate Setting 1, Audio Streaming - Operational */
		  .bLength = sizeof(audio_desc.ADID12),
		  .bDescriptorType = USB_DESC_TYPE_INTERFACE,
		  .bInterfaceNumber = 0x02,
		  .bAlternateSetting = 0x01,
		  .bNumEndpoints = 0x01,
		  .bInterfaceClass = USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass: Communication Interface Class */
		  .bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass: Abstract Control Model */
		  .bInterfaceProtocol = 0, /* bInterfaceProtocol: Common AT commands */
				  .iInterface = 0,
	  },
		  .ASID2 = { /* Audio Streaming Interface */
		  .bLength = sizeof(audio_desc.ASID2),
		  .bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		  .bDescriptorSubtype = AUDIO_SUBCLASS_AUDIOCONTROL, /* bDescriptorSubtype: Header Func Desc */
		  .bTerminalLink = 0x05,
				  .bDelay = 0x01,
				  .wFormatTagLSB = 0x01,
				  .wFormatTagMSB = 0x00,
	  },
	  .ASFID2 = { /* Audio Type I Format */
		  .bLength = sizeof(audio_desc.ASFID2),
		  .bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
		  .bDescriptorSubtype = AUDIO_STREAMING_FORMAT_TYPE,
				  .bFormatType = AUDIO_FORMAT_TYPE_I,
		  .bNrChannels = 0x02,
				  .bSubFrameSize = 0x02,
				  .bBitResolution = 0x10, //16 bit
				  .bSamFreqType = 0x01,
				  .afreq1 = (uint8_t)(USBD_AUDIO_FREQ),
				  .afreq2 = (uint8_t)((USBD_AUDIO_FREQ >> 8)),
				  .afreq3 = (uint8_t)((USBD_AUDIO_FREQ >> 16)),
	  },

	  /* Endpoint 1 - Standard Descriptor */
			.EP12 = {
			.bLength = sizeof(audio_desc.EP12),
			.bDescriptorType = USB_DESC_TYPE_ENDPOINT, /* bDescriptorType: CS_INTERFACE */
			.bEndpointAddress = 0x86,
					.bmAttributes = USB_EP_TYPE_ISOCHRONOUS,
			.wMaxPacketSize = AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),
					.bInterval = 0x01,
					.bRefresh = 0x00,
					.bSynchAddress = 0x00,
		},
			.EP1D2 = { /* USB Audio Data Endpoint Descriptor */
			.bLength = sizeof(audio_desc.EP1D2),
			.bDescriptorType = AUDIO_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: CS_INTERFACE */
			.bDescriptorSubType = AUDIO_ENDPOINT_GENERAL,
					.bmAttributes = 0x00,
			.bLockDelayUnits = 0x00,
					.wLockDelay = 0x00,
			.tmp = 0x00,
		},
};

static uint16_t USBD_AUDIO_GetDesc(USBD_AUDIO_IfHandleType *itf, uint8_t ifNum, uint8_t * dest)
{
	USBD_AUDIO_DescType *desc = (USBD_AUDIO_DescType*)dest;
	uint16_t len = sizeof(audio_desc);

	memcpy(dest, &audio_desc, sizeof(audio_desc));

	/* Adjustment of interface indexes */
	desc->IADA.bFirstInterface = ifNum;
	desc->IADA.iFunction  = USBD_IIF_INDEX(ifNum, 0);

	desc->ACIF.bInterfaceNumber = ifNum;
	desc->ACI.baInterfaceNr = ifNum + 1;
	desc->ADID0.bInterfaceNumber = ifNum + 1;
	desc->ADID1.bInterfaceNumber = ifNum + 1;
	desc->ACI.baInterfaceNr2 = ifNum + 2;
	desc->ADID02.bInterfaceNumber = ifNum + 2;
	desc->ADID12.bInterfaceNumber = ifNum + 2;

	//desc->CIF.iInterface = USBD_IIF_INDEX(ifNum, 0);
//desc->ADID0.iInterface = USBD_IIF_INDEX(ifNum, 0);
	//desc->ADID1.iInterface = USBD_IIF_INDEX(ifNum, 0);

	desc->EP1.bEndpointAddress = itf->Config.OutEpNum;
	desc->EP12.bEndpointAddress = itf->Config.InEpNum;

	return len;
}

/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static void USBD_AUDIO_Init(USBD_AUDIO_IfHandleType *pdev)
{
	if (pdev->Base.AltSelector == 1)
    {
      USBD_HandleType *dev = pdev->Base.Device;
			USBD_AUDIO_HandleTypeDef   *haudio;
			
			/* Open EP OUT */
			USBD_EpOpen(dev, pdev->Config.OutEpNum, USB_EP_TYPE_ISOCHRONOUS, AUDIO_OUT_PACKET);
			USBD_EpOpen(dev, pdev->Config.InEpNum, USB_EP_TYPE_ISOCHRONOUS, AUDIO_OUT_PACKET);

			/* Allocate Audio structure */
			haudio = (USBD_AUDIO_HandleTypeDef*)&pdev->Config.handle;
			haudio->alt_setting = 0U;
			haudio->offset = AUDIO_OFFSET_UNKNOWN;
			haudio->wr_ptr = 0U;
			haudio->rd_ptr = 0U;
			haudio->rd_enable = 0U;

			/* Initialize the Audio output Hardware layer */
			USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->Init, pdev, USBD_AUDIO_FREQ, AUDIO_DEFAULT_VOLUME, 0U);

			/* Prepare Out endpoint to receive 1st packet */
			USBD_EpReceive(pdev->Base.Device, pdev->Config.OutEpNum, haudio->buffer, AUDIO_OUT_PACKET);
    }
    else
    {
        /* TODO reset MAC address to default */
    }
}

/**
  * @brief  USBD_AUDIO_Init
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static void  USBD_AUDIO_DeInit(USBD_AUDIO_IfHandleType *pdev)
{
	if (pdev->Base.AltSelector == 1)
  {
		USBD_HandleType *dev = pdev->Base.Device;
		USBD_EpClose(dev, pdev->Config.OutEpNum);
		USBD_EpClose(dev, pdev->Config.InEpNum);
		USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->DeInit, pdev, 0U);
	}
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_AUDIO_Setup(USBD_AUDIO_IfHandleType *pdev)
{
	USBD_AUDIO_HandleTypeDef *haudio;
	uint16_t len;
	uint8_t *pbuf;
	uint16_t status_info = 0U;
	USBD_ReturnType retval = USBD_E_INVALID;
	USBD_HandleType *dev = pdev->Base.Device;
	USB_SetupRequestType req = dev->Setup;

	switch (dev->Setup.RequestType.Type)
	{
	case USB_REQ_TYPE_CLASS:
		switch (dev->Setup.Request)
		{
		case AUDIO_REQ_GET_CUR:
			retval = AUDIO_REQ_GetCurrent(pdev, &req);
			break;

		case AUDIO_REQ_SET_CUR:
			retval = AUDIO_REQ_SetCurrent(pdev, &req);
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

static const char* USBD_AUDIO_GetString(USBD_AUDIO_IfHandleType *itf, uint8_t intNum)
{
	return itf->App->Name;
}

static void USBD_AUDIO_DataStage(USBD_AUDIO_IfHandleType *itf)
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
static uint8_t  USBD_AUDIO_DataIn(USBD_AUDIO_IfHandleType *pdev,	uint8_t epnum)
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
static uint8_t  USBD_AUDIO_EP0_RxReady(USBD_AUDIO_IfHandleType *pdev)
{
	USBD_AUDIO_HandleTypeDef   *haudio;
	haudio = (USBD_AUDIO_HandleTypeDef*)&pdev->Config.handle;

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
static uint8_t  USBD_AUDIO_EP0_TxReady(USBD_AUDIO_IfHandleType *pdev)
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
static uint8_t  USBD_AUDIO_SOF(USBD_AUDIO_IfHandleType *pdev)
{
	return USBD_E_OK;
}

/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
void  USBD_AUDIO_Sync(USBD_AUDIO_IfHandleType *pdev, AUDIO_OffsetTypeDef offset)
{
	uint32_t cmd = 0U;
	USBD_AUDIO_HandleTypeDef   *haudio;
	haudio = (USBD_AUDIO_HandleTypeDef*)&pdev->Config.handle;

	haudio->offset = offset;

	if (haudio->rd_enable == 1U)
	{
		haudio->rd_ptr += (uint16_t)(AUDIO_TOTAL_BUF_SIZE / 2U);

		if (haudio->rd_ptr == AUDIO_TOTAL_BUF_SIZE)
		{
			/* roll back */
			haudio->rd_ptr = 0U;
		}
	}

	if (haudio->rd_ptr > haudio->wr_ptr)
	{
		if ((haudio->rd_ptr - haudio->wr_ptr) < AUDIO_OUT_PACKET)
		{
			cmd = AUDIO_TOTAL_BUF_SIZE / 2U + 4U;
		}
		else
		{
			if ((haudio->rd_ptr - haudio->wr_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
			{
				cmd = AUDIO_TOTAL_BUF_SIZE / 2U - 4U;
			}
		}
	}
	else
	{
		if ((haudio->wr_ptr - haudio->rd_ptr) < AUDIO_OUT_PACKET)
		{
			cmd = AUDIO_TOTAL_BUF_SIZE / 2U - 4U;
		}
		else
		{
			if ((haudio->wr_ptr - haudio->rd_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
			{
				cmd = AUDIO_TOTAL_BUF_SIZE / 2U + 4U;
			}
		}
	}

	if (haudio->offset == AUDIO_OFFSET_FULL)
	{
		USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->AudioCmd, pdev, &haudio->buffer[0], cmd, AUDIO_CMD_PLAY);
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
static uint8_t  USBD_AUDIO_DataOut(USBD_AUDIO_IfHandleType *pdev,	uint8_t epnum)
{
	USBD_AUDIO_HandleTypeDef   *haudio;
	haudio = (USBD_AUDIO_HandleTypeDef*)&pdev->Config.handle;

	if (epnum == pdev->Config.OutEpNum)
	{
		/* Increment the Buffer pointer or roll it back when all buffers are full */

		haudio->wr_ptr += AUDIO_OUT_PACKET;

		if (haudio->wr_ptr == AUDIO_TOTAL_BUF_SIZE)
		{
			/* All buffers are full: roll back */
			haudio->wr_ptr = 0U;

			if (haudio->offset == AUDIO_OFFSET_UNKNOWN)
			{
				USBD_SAFE_CALLBACK(AUDIO_APP(pdev)->AudioCmd, pdev, &haudio->buffer[0], AUDIO_TOTAL_BUF_SIZE / 2U, AUDIO_CMD_START);
				haudio->offset = AUDIO_OFFSET_NONE;
			}
		}

		if (haudio->rd_enable == 0U)
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
static USBD_ReturnType AUDIO_REQ_GetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req)
{
	USBD_AUDIO_HandleTypeDef   *haudio;
	haudio = (USBD_AUDIO_HandleTypeDef*)&pdev->Config.handle;

	memset(haudio->control.data, 0, 64U);
	
	/* Send the current mute state */
	return USBD_CtrlSendData(pdev->Base.Device, haudio->control.data, req->Length);
}

/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static USBD_ReturnType AUDIO_REQ_SetCurrent(USBD_AUDIO_IfHandleType *pdev, USB_SetupRequestType *req)
{
	USBD_ReturnType retval = USBD_E_INVALID;
	USBD_AUDIO_HandleTypeDef   *haudio;
	haudio = (USBD_AUDIO_HandleTypeDef*)&pdev->Config.handle;

	if (req->Length)
	{
		/* Prepare the reception of the buffer over EP0 */
		retval = USBD_CtrlReceiveData(pdev->Base.Device, haudio->control.data);

		haudio->control.cmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
		haudio->control.len = (uint8_t)req->Length; /* Set the request data length */
		haudio->control.unit = HIBYTE(req->Index);  /* Set the request target unit */
	}
	
	return retval;
}

/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t  USBD_AUDIO_RegisterInterface(USBD_AUDIO_IfHandleType   *pdev, USBD_AUDIO_AppType *fops)
{
	if (fops != NULL)
	{
		pdev->App = fops;
	}
	return USBD_E_OK;
}

USBD_ReturnType USBD_AUDIO_MountInterface(USBD_AUDIO_IfHandleType *itf, USBD_HandleType *dev)
{
	USBD_ReturnType retval = USBD_E_ERROR;

	/* Note: AUDIO IN-OUT uses 3 interfaces */
	if (dev->IfCount < (USBD_MAX_IF_COUNT - 2))
	{
		/* Binding interfaces */
		itf->Base.Device = dev;
		itf->Base.Class = &audio_cbks;
		itf->Base.AltCount = 2;
		itf->Base.AltSelector = 0;

		USBD_EpHandleType *ep;
		
		ep= USBD_EpAddr2Ref(dev, itf->Config.OutEpNum);
		ep->Type = USB_EP_TYPE_ISOCHRONOUS;
		ep->MaxPacketSize = AUDIO_OUT_PACKET;
		ep->IfNum = dev->IfCount;
		
		ep = USBD_EpAddr2Ref(dev, itf->Config.InEpNum);
		ep->Type = USB_EP_TYPE_ISOCHRONOUS;
		ep->MaxPacketSize = AUDIO_OUT_PACKET;
		ep->IfNum = dev->IfCount;
		
		dev->IF[dev->IfCount] = (USBD_IfHandleType*)itf;
		dev->IfCount++;

		dev->IF[dev->IfCount] = (USBD_IfHandleType*)itf;
		dev->IfCount++;
		
		dev->IF[dev->IfCount] = (USBD_IfHandleType*)itf;
		dev->IfCount++;
		
		retval = USBD_E_OK;
	}

	return retval;
}

