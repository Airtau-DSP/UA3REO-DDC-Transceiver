/* Includes ------------------------------------------------------------------*/
#include "usbd_ua3reo.h"
#include "usbd_ctlreq.h"
#include "functions.h"

static uint8_t  USBD_UA3REO_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_UA3REO_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_UA3REO_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  USBD_UA3REO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_UA3REO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_UA3REO_EP0_RxReady (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_UA3REO_EP0_TxReady (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_UA3REO_SOF (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_UA3REO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_UA3REO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  *USBD_UA3REO_GetFSCfgDesc (uint16_t *length);
static uint8_t  *USBD_UA3REO_GetOtherSpeedCfgDesc (uint16_t *length);
uint8_t  *USBD_UA3REO_GetDeviceQualifierDescriptor (uint16_t *length);

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_UA3REO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/* UA3REO interface class callbacks structure */
USBD_ClassTypeDef  USBD_UA3REO =
{
  USBD_UA3REO_Init,
  USBD_UA3REO_DeInit,
  USBD_UA3REO_Setup,
  USBD_UA3REO_EP0_TxReady,
  USBD_UA3REO_EP0_RxReady,
  USBD_UA3REO_DataIn,
  USBD_UA3REO_DataOut,
  USBD_UA3REO_SOF,
  USBD_UA3REO_IsoINIncomplete,
  USBD_UA3REO_IsoOutIncomplete,
  NULL,
  USBD_UA3REO_GetFSCfgDesc,
  NULL,
  USBD_UA3REO_GetDeviceQualifierDescriptor,
};

/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_UA3REO_CfgFSDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /*Configuration Descriptor*/
  0x09,   /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,      /* bDescriptorType: Configuration */
  USB_CDC_CONFIG_DESC_SIZ,                /* wTotalLength:no of returned bytes */
  0x00,
  0x06,   /* bNumInterfaces: count interface */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  0xC0,   /* bmAttributes: self powered */
  0xFA,   /* MaxPower 500 mA */

  /*---------------------------------------------------------------------------*/
	
	//DEBUG/KEY PORT
	//Interface Association Descriptor:
	//------------------------------
	0x08,     //bLength
	0x0B,     //bDescriptorType
	0x00,     //bFirstInterface
	0x02,     //bInterfaceCount
	0x02,     //bFunctionClass      (Communication Device Class)
	0x02,     //bFunctionSubClass   (Abstract Control Model - ACM)
	0x01,     //bFunctionProtocol   (ITU-T V.250)
	USBD_IDX_INTERFACE1_STR,     //iFunction   ""

  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  //0x01,   /* bNumEndpoints: One endpoints used */
	0x00,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  USBD_IDX_INTERFACE1_STR,   /* iInterface: */

  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,

  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */

  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */

  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x00,   /* bMasterInterface: Communication class interface */
  0x01,   /* bSlaveInterface0: Data Class Interface */

  /*Endpoint 2 Descriptor*/
	/*
  0x07,                           // bLength: Endpoint Descriptor size 
  USB_DESC_TYPE_ENDPOINT,   // bDescriptorType: Endpoint 
  DEBUG_CMD_EP,                     // bEndpointAddress 
  0x03,                           // bmAttributes: Interrupt 
  LOBYTE(CDC_CMD_PACKET_SIZE),     // wMaxPacketSize: 
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                           // bInterval: 
	*/
  /*---------------------------------------------------------------------------*/

  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  USBD_IDX_INTERFACE1_STR,   /* iInterface: */

  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  DEBUG_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x01,                              /* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  DEBUG_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x01,                               /* bInterval: ignore for Bulk transfer */

	//CAT PORT
	//Interface Association Descriptor:
	//------------------------------
	0x08,     //bLength
	0x0B,     //bDescriptorType
	0x02,     //bFirstInterface
	0x02,     //bInterfaceCount
	0x02,     //bFunctionClass      (Communication Device Class)
	0x02,     //bFunctionSubClass   (Abstract Control Model - ACM)
	0x01,     //bFunctionProtocol   (ITU-T V.250)
	USBD_IDX_INTERFACE2_STR,     //iFunction   ""
	
	/*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x02,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  //0x01,   /* bNumEndpoints: One endpoints used */
	0x00,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  USBD_IDX_INTERFACE2_STR,   /* iInterface: */
	
	/*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,

  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x03,   /* bDataInterface: 1 */

  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */

  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x02,   /* bMasterInterface: Communication class interface */
  0x03,   /* bSlaveInterface0: Data Class Interface */
	
	/*Endpoint 2 Descriptor*/
	/*
  0x07,                           // bLength: Endpoint Descriptor size 
  USB_DESC_TYPE_ENDPOINT,   // bDescriptorType: Endpoint 
  CAT_CMD_EP,                     // bEndpointAddress 
  0x03,                           // bmAttributes: Interrupt 
  LOBYTE(CDC_CMD_PACKET_SIZE),     // wMaxPacketSize: 
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_FS_BINTERVAL,                           // bInterval:
	*/
  /*---------------------------------------------------------------------------*/
	
	/*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  0x03,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  USBD_IDX_INTERFACE2_STR,   /* iInterface: */
	
	/*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CAT_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x01,                              /* bInterval: ignore for Bulk transfer */

  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CAT_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x01,                               /* bInterval: ignore for Bulk transfer */
	
	//AUDIO RX PORT
	//Interface Association Descriptor:
	//------------------------------
	0x08,//     bLength
	0x0B,//    bDescriptorType
	0x04,//     bFirstInterface
	0x02,//     bInterfaceCount
	0x01,//     bFunctionClass      (Audio Device Class)
	0x00,//     bFunctionSubClass  
	0x00,//     bFunctionProtocol  
	USBD_IDX_INTERFACE3_STR,//     iFunction   ""

	//Interface Descriptor:
	//------------------------------
	0x09,//     bLength
	0x04,//     bDescriptorType
	0x04,//     bInterfaceNumber
	0x00,//     bAlternateSetting
	0x00,//     bNumEndPoints
	0x01,//     bInterfaceClass      (Audio Device Class)
	0x01,//     bInterfaceSubClass   (Audio Control Interface)
	0x00,//     bInterfaceProtocol  
	USBD_IDX_INTERFACE3_STR,//     iInterface   ""

	//AC Interface Header Descriptor:
	//------------------------------
	0x09,//     bLength
	0x24,//     bDescriptorType
	0x01,//     bDescriptorSubtype
	0x00,//
	0x01,// bcdADC
	0x1E,// wTotalLength   (30 bytes)
	0x00,// wTotalLength   (30 bytes)
	0x01,//     bInCollection
	0x05,//     baInterfaceNr(1)

	//AC Input Terminal Descriptor:
	//------------------------------
	0x0C,//    bLength
	0x24,//     bDescriptorType
	0x02,//     bDescriptorSubtype
	0x1E,//    bTerminalID
	0x10,// wTerminalType   (Radio Receiver)
	0x07,// wTerminalType   (Radio Receiver)
	0x00,//     bAssocTerminal
	0x02,//     bNrChannels   (2 channels)
	0x03,// wChannelConfig
	0x00,// wChannelConfig
	0x00,//    iChannelNames
	0x00,//     iTerminal
	 
	//AC Output Terminal Descriptor:
	//------------------------------
	0x09,//     bLength
	0x24,//     bDescriptorType
	0x03,//     bDescriptorSubtype
	0x23,//     bTerminalID
	0x01,// wTerminalType   (USB Streaming)
	0x01,// wTerminalType   (USB Streaming)
	0x00,//     bAssocTerminal
	0x1E,//    bSourceID
	0x00,//     iTerminal   "wwww 1"

	//Interface Descriptor:
	//------------------------------
	0x09,//     bLength
	0x04,//     bDescriptorType
	0x05,//     bInterfaceNumber
	0x00,//     bAlternateSetting
	0x00,//     bNumEndPoints
	0x01,//     bInterfaceClass      (Audio Device Class)
	0x02,//     bInterfaceSubClass   (Audio Streaming Interface)
	0x00,//     bInterfaceProtocol  
	USBD_IDX_INTERFACE3_STR,//     iInterface   "xxxx 1"

	//Interface Descriptor:
	//------------------------------
	0x09,//     bLength
	0x04,//     bDescriptorType
	0x05,//     bInterfaceNumber
	0x01,//     bAlternateSetting
	0x01,//     bNumEndPoints
	0x01,//     bInterfaceClass      (Audio Device Class)
	0x02,//     bInterfaceSubClass   (Audio Streaming Interface)
	0x00,//     bInterfaceProtocol  
	USBD_IDX_INTERFACE3_STR,//     iInterface   "xxxx 1"

	//AS Interface Descriptor:
	//------------------------------
	0x07,//     bLength
	0x24,//     bDescriptorType
	0x01,//     bDescriptorSubtype
	0x23,//     bTerminalLink
	0x01,//     bDelay
	0x01,// wFormatTag   (PCM)
	0x00,// wFormatTag   (PCM)

	//AS Format Type 1 Descriptor:
	//------------------------------
	0x0B,//    bLength
	0x24,//     bDescriptorType
	0x02,//     bDescriptorSubtype
	0x01,//     bFormatType   (FORMAT_TYPE_1)
	0x02,//     bNrChannels   (2 channels)
	0x02,//     bSubframeSize
	0x10,//     bBitResolution   (16 bits per sample)
	0x01,//     bSamFreqType   (Discrete sampling frequencies)
	0x80,//         tSamFreq(1)   (48000 Hz)
	0xBB,//         tSamFreq(1)   (48000 Hz)
	0x00,//         tSamFreq(1)   (48000 Hz)

	//Endpoint Descriptor (Audio/MIDI 1.0):
	//------------------------------
	0x09,//     bLength
	0x05,//     bDescriptorType
	AUDIO_IN_EP,//     bEndpointAddress  (IN endpoint 1)
	0x25,//     bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Implicit)
	0xC4,// wMaxPacketSize    (1 x 196 bytes)
	0x00,// wMaxPacketSize    (1 x 196 bytes)
	0x01,//     bInterval         (1 frames)
	0x00,//     bRefresh
	0x00,//     bSynchAddress

	//AS Isochronous Data Endpoint Descriptor:
	//------------------------------
	0x07,//     bLength
	0x25,//     bDescriptorType
	0x01,//     bDescriptorSubtype
	0x00,//     bmAttributes
	0x02,//     bLockDelayUnits   (decoded PCM samples)
	0x00,// wLockDelay
	0x00,// wLockDelay
} ;

/**
  * @}
  */

/** @defgroup USBD_CDC_Private_Functions
  * @{
  */

/**
  * @brief  USBD_CDC_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_UA3REO_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0U;
  USBD_DEBUG_HandleTypeDef   *hcdc_debug;
	USBD_CAT_HandleTypeDef   *hcdc_cat;
	USBD_AUDIO_HandleTypeDef   *haudio;

  /* Open EP IN */
  USBD_LL_OpenEP(pdev, DEBUG_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_IN_PACKET_SIZE);
	USBD_LL_OpenEP(pdev, CAT_IN_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_IN_PACKET_SIZE);
	USBD_LL_OpenEP(pdev, AUDIO_IN_EP, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET);

  pdev->ep_in[DEBUG_IN_EP & 0xFU].is_used = 1U;
	pdev->ep_in[CAT_IN_EP & 0xFU].is_used = 1U;
	pdev->ep_in[AUDIO_IN_EP & 0xFU].is_used = 1U;

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev, DEBUG_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_OUT_PACKET_SIZE);
	USBD_LL_OpenEP(pdev, CAT_OUT_EP, USBD_EP_TYPE_BULK, CDC_DATA_FS_OUT_PACKET_SIZE);

  pdev->ep_out[DEBUG_OUT_EP & 0xFU].is_used = 1U;
	pdev->ep_out[CAT_OUT_EP & 0xFU].is_used = 1U;
  
  /* Open Command IN EP */
  //USBD_LL_OpenEP(pdev, DEBUG_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
	//USBD_LL_OpenEP(pdev, CAT_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
  //pdev->ep_in[DEBUG_CMD_EP & 0xFU].is_used = 1U;
	//pdev->ep_in[CAT_CMD_EP & 0xFU].is_used = 1U;

  pdev->pClassDataDEBUG = USBD_malloc(sizeof (USBD_DEBUG_HandleTypeDef));
	pdev->pClassDataCAT = USBD_malloc(sizeof (USBD_CAT_HandleTypeDef));
	pdev->pClassDataAUDIO = malloc(sizeof (USBD_AUDIO_HandleTypeDef));

  if(pdev->pClassDataDEBUG == NULL)
  {
    ret = 1U;
		return ret;
  }
  else
  {
    hcdc_debug = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;

    /* Init  physical Interface components */
    ((USBD_DEBUG_ItfTypeDef *)pdev->pUserDataDEBUG)->Init();

    /* Init Xfer states */
    hcdc_debug->TxState = 0U;
    hcdc_debug->RxState = 0U;

    /* Prepare Out endpoint to receive next packet */
    USBD_LL_PrepareReceive(pdev, DEBUG_OUT_EP, hcdc_debug->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);
  }
	
	if(pdev->pClassDataCAT == NULL)
  {
    ret = 1U;
		return ret;
  }
  else
  {
    hcdc_cat = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;

    ((USBD_CAT_ItfTypeDef *)pdev->pUserDataCAT)->Init();

    hcdc_cat->TxState = 0U;
    hcdc_cat->RxState = 0U;

    USBD_LL_PrepareReceive(pdev, CAT_OUT_EP, hcdc_cat->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);
  }
	
	if(pdev->pClassDataAUDIO == NULL)
  {
    ret = 1U;
		return ret;
  }
  else
  {
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassDataAUDIO;
    haudio->alt_setting = 0U;
    haudio->offset = AUDIO_OFFSET_UNKNOWN;
    haudio->wr_ptr = 0U;
    haudio->rd_ptr = 0U;
    haudio->rd_enable = 0U;
		
    // Initialize the Audio output Hardware layer
    if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserDataAUDIO)->Init(USBD_AUDIO_FREQ, AUDIO_DEFAULT_VOLUME, 0U) != 0)
    {
      return USBD_FAIL;
    }

    //Prepare Out endpoint to receive 1st packet
    //USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, haudio->buffer, AUDIO_OUT_PACKET);
  }

  return ret;
}

/**
  * @brief  USBD_CDC_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_UA3REO_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0U;

  /* Close EP IN */
  USBD_LL_CloseEP(pdev, DEBUG_IN_EP);
	USBD_LL_CloseEP(pdev, CAT_IN_EP);
	USBD_LL_CloseEP(pdev, AUDIO_IN_EP);
  pdev->ep_in[DEBUG_IN_EP & 0xFU].is_used = 0U;
	pdev->ep_in[CAT_IN_EP & 0xFU].is_used = 0U;
	pdev->ep_in[AUDIO_IN_EP & 0xFU].is_used = 0U;

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, DEBUG_OUT_EP);
	USBD_LL_CloseEP(pdev, CAT_OUT_EP);
  pdev->ep_out[DEBUG_OUT_EP & 0xFU].is_used = 0U;
	pdev->ep_out[CAT_OUT_EP & 0xFU].is_used = 0U;

  /* Close Command IN EP */
  //USBD_LL_CloseEP(pdev, DEBUG_CMD_EP);
	//USBD_LL_CloseEP(pdev, CAT_CMD_EP);
  //pdev->ep_in[DEBUG_CMD_EP & 0xFU].is_used = 0U;
	//pdev->ep_in[CAT_CMD_EP & 0xFU].is_used = 0U;

  /* DeInit  physical Interface components */
  if(pdev->pClassDataDEBUG != NULL)
  {
    ((USBD_DEBUG_ItfTypeDef *)pdev->pUserDataDEBUG)->DeInit();
    USBD_free(pdev->pClassDataDEBUG);
    pdev->pClassDataDEBUG = NULL;
  }
	if(pdev->pClassDataCAT != NULL)
  {
    ((USBD_CAT_ItfTypeDef *)pdev->pUserDataCAT)->DeInit();
    USBD_free(pdev->pClassDataCAT);
    pdev->pClassDataCAT = NULL;
  }
	if(pdev->pClassDataAUDIO != NULL)
  {
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserDataAUDIO)->DeInit(0U);
    USBD_free(pdev->pClassDataAUDIO);
    pdev->pClassDataAUDIO = NULL;
  }
  return ret;
}

/**
  * @brief  USBD_CDC_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_DEBUG_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_DEBUG_HandleTypeDef   *hcdc_debug = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    if (req->wLength)
    {
      if (req->bmRequest & 0x80U)
      {
        ((USBD_DEBUG_ItfTypeDef *)pdev->pUserDataDEBUG)->Control(req->bRequest, (uint8_t *)(void *)hcdc_debug->data, req->wLength);
				USBD_CtlSendData (pdev, (uint8_t *)(void *)hcdc_debug->data, req->wLength);
      }
      else
      {
        hcdc_debug->CmdOpCode = req->bRequest;
        hcdc_debug->CmdLength = (uint8_t)req->wLength;

        USBD_CtlPrepareRx (pdev, (uint8_t *)(void *)hcdc_debug->data, req->wLength);
      }
    }
    else
    {
      ((USBD_DEBUG_ItfTypeDef *)pdev->pUserDataDEBUG)->Control(req->bRequest, (uint8_t *)(void *)req, 0U);
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, &ifalt, 1U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE:
      if (pdev->dev_state != USBD_STATE_CONFIGURED)
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;

  default:
    USBD_CtlError (pdev, req);
    ret = USBD_FAIL;
    break;
  }
  return ret;
}
static uint8_t  USBD_CAT_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_CAT_HandleTypeDef   *hcdc_cat = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
	USBD_CAT_HandleTypeDef   *hcdc_debug = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    if (req->wLength)
    {
      if (req->bmRequest & 0x80U)
      {
        ((USBD_CAT_ItfTypeDef *)pdev->pUserDataCAT)->Control(req->bRequest, (uint8_t *)(void *)hcdc_cat->data, req->wLength);
				USBD_CtlSendData (pdev, (uint8_t *)(void *)hcdc_cat->data, req->wLength);
      }
      else
      {
        hcdc_cat->CmdOpCode = req->bRequest;
        hcdc_cat->CmdLength = (uint8_t)req->wLength;
        USBD_CtlPrepareRx (pdev, (uint8_t *)(void *)hcdc_cat->data, req->wLength);
      }
    }
    else
    {
      ((USBD_CAT_ItfTypeDef *)pdev->pUserDataCAT)->Control(req->bRequest, (uint8_t *)(void *)req, 0U);
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, &ifalt, 1U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE:
      if (pdev->dev_state != USBD_STATE_CONFIGURED)
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;

  default:
    USBD_CtlError (pdev, req);
    ret = USBD_FAIL;
    break;
  }
  return ret;
}
static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassDataAUDIO;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {
    case AUDIO_REQ_GET_CUR:
      AUDIO_REQ_GetCurrent(pdev, req);
      break;

    case AUDIO_REQ_SET_CUR:
      AUDIO_REQ_SetCurrent(pdev, req);
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError (pdev, req);
                         ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_DESCRIPTOR:
      if( (req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
      {
        //pbuf = USBD_AUDIO_CfgDesc + 18;
        //len = MIN(USB_AUDIO_DESC_SIZ , req->wLength);
        //USBD_CtlSendData (pdev, pbuf, len);
      }
      break;

    case USB_REQ_GET_INTERFACE :
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&haudio->alt_setting, 1U);
      }
      else
      {
        USBD_CtlError (pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE :
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
         if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES)
         {
           haudio->alt_setting = (uint8_t)(req->wValue);
         }
         else
         {
           /* Call the error management function (command will be nacked */
           USBD_CtlError (pdev, req);
           ret = USBD_FAIL;
         }
      }
      else
      {
        USBD_CtlError (pdev, req);
        ret = USBD_FAIL;
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;
  default:
    USBD_CtlError (pdev, req);
    ret = USBD_FAIL;
    break;
  }
  return ret;
}
static uint8_t  USBD_UA3REO_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	uint8_t ret=0;
	// Route requests to MSC interface or its endpoints to MSC class implementaion
	if(((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE && req->wIndex == DEBUG_INTERFACE_IDX) ||
		((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_ENDPOINT && ((req->wIndex & 0x7F) == DEBUG_EP_IDX)))
	{
		ret = USBD_DEBUG_Setup(pdev, req);
	}
	if(((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE && req->wIndex == AUDIO_INTERFACE_IDX) ||
		((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_ENDPOINT && ((req->wIndex & 0x7F) == AUDIO_EP_IDX)))
	{
		ret = USBD_AUDIO_Setup(pdev, req);
	}
	ret = USBD_CAT_Setup(pdev, req);
	//sendToDebug_uint8(ret,false);
	return ret;
}
/**
  * @brief  USBD_CDC_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_DEBUG_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_DEBUG_HandleTypeDef *hcdc_debug = (USBD_DEBUG_HandleTypeDef*)pdev->pClassDataDEBUG;
  PCD_HandleTypeDef *hpcd_debug = pdev->pUserDataDEBUG;

  if(pdev->pClassDataDEBUG != NULL)
  {
    if((pdev->ep_in[epnum].total_length > 0U) && ((pdev->ep_in[epnum].total_length % hpcd_debug->IN_ep[epnum].maxpacket) == 0U))
    {
      /* Update the packet total length */
      pdev->ep_in[epnum].total_length = 0U;

      /* Send ZLP */
      USBD_LL_Transmit (pdev, epnum, NULL, 0U);
    }
    else
    {
      hcdc_debug->TxState = 0U;
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
static uint8_t  USBD_CAT_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	USBD_CAT_HandleTypeDef *hcdc_cat = (USBD_CAT_HandleTypeDef*)pdev->pClassDataCAT;
	PCD_HandleTypeDef *hpcd_cat = pdev->pUserDataCAT;

  if(pdev->pClassDataCAT != NULL)
  {
    if((pdev->ep_in[epnum].total_length > 0U) && ((pdev->ep_in[epnum].total_length % hpcd_cat->IN_ep[epnum].maxpacket) == 0U))
    {
      /* Update the packet total length */
      pdev->ep_in[epnum].total_length = 0U;

      /* Send ZLP */
      USBD_LL_Transmit (pdev, epnum, NULL, 0U);
    }
    else
    {
      hcdc_cat->TxState = 0U;
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  /* Only OUT data are processed */
  return USBD_OK;
}
static uint8_t  USBD_UA3REO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  if(epnum == DEBUG_EP_IDX) return USBD_DEBUG_DataIn(pdev, epnum);
	if(epnum == CAT_EP_IDX) return USBD_CAT_DataIn(pdev, epnum);
	if(epnum == AUDIO_EP_IDX) return USBD_AUDIO_DataIn(pdev, epnum);
	return USBD_FAIL;
}
/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_DEBUG_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_DEBUG_HandleTypeDef   *hcdc_debug = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;
  /* Get the received data length */
  hcdc_debug->RxLength = USBD_LL_GetRxDataSize (pdev, epnum);
  /* USB data will be immediately processed, this allow next USB traffic being
  NAKed till the end of the application Xfer */
  if(pdev->pClassDataDEBUG != NULL)
  {
    ((USBD_DEBUG_ItfTypeDef *)pdev->pUserDataDEBUG)->Receive(hcdc_debug->RxBuffer, &hcdc_debug->RxLength);
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
static uint8_t  USBD_CAT_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	USBD_CAT_HandleTypeDef   *hcdc_cat = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
  /* Get the received data length */
  hcdc_cat->RxLength = USBD_LL_GetRxDataSize (pdev, epnum);
  /* USB data will be immediately processed, this allow next USB traffic being
  NAKed till the end of the application Xfer */
  if(pdev->pClassDataCAT != NULL)
  {
    ((USBD_CAT_ItfTypeDef *)pdev->pUserDataCAT)->Receive(hcdc_cat->RxBuffer, &hcdc_cat->RxLength);
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassDataAUDIO;

  if (epnum == AUDIO_OUT_EP)
  {
    /* Increment the Buffer pointer or roll it back when all buffers are full */

    haudio->wr_ptr += AUDIO_OUT_PACKET;

    if (haudio->wr_ptr == AUDIO_TOTAL_BUF_SIZE)
    {
      /* All buffers are full: roll back */
      haudio->wr_ptr = 0U;

      if(haudio->offset == AUDIO_OFFSET_UNKNOWN)
      {
        ((USBD_AUDIO_ItfTypeDef *)pdev->pUserDataAUDIO)->AudioCmd(&haudio->buffer[0],
                                                             AUDIO_TOTAL_BUF_SIZE / 2U,
                                                             AUDIO_CMD_START);
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
    //USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, &haudio->buffer[haudio->wr_ptr], AUDIO_OUT_PACKET);
  }

  return USBD_OK;
}
static uint8_t  USBD_UA3REO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  if(epnum == DEBUG_EP_IDX) return USBD_DEBUG_DataOut(pdev, epnum);
	if(epnum == CAT_EP_IDX) return USBD_CAT_DataOut(pdev, epnum);
	if(epnum == AUDIO_EP_IDX) return USBD_AUDIO_DataOut(pdev, epnum);
	return USBD_FAIL;
}

/**
  * @brief  USBD_CDC_EP0_RxReady
  *         Handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_UA3REO_EP0_RxReady (USBD_HandleTypeDef *pdev)
{
  USBD_DEBUG_HandleTypeDef   *hcdc_debug = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;
	USBD_CAT_HandleTypeDef   *hcdc_cat = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
	//DEBUG
  if((pdev->pUserDataDEBUG != NULL) && (hcdc_debug->CmdOpCode != 0xFFU))
  {
    ((USBD_DEBUG_ItfTypeDef *)pdev->pUserDataDEBUG)->Control(hcdc_debug->CmdOpCode, (uint8_t *)(void *)hcdc_debug->data, (uint16_t)hcdc_debug->CmdLength);
    hcdc_debug->CmdOpCode = 0xFFU;
  }
	//CAT
	if((pdev->pUserDataCAT != NULL) && (hcdc_cat->CmdOpCode != 0xFFU))
  {
    ((USBD_CAT_ItfTypeDef *)pdev->pUserDataCAT)->Control(hcdc_cat->CmdOpCode, (uint8_t *)(void *)hcdc_cat->data, (uint16_t)hcdc_cat->CmdLength);
    hcdc_cat->CmdOpCode = 0xFFU;
  }
	//AUDIO
	USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassDataAUDIO;

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR)
  {
    if (haudio->control.unit == AUDIO_OUT_STREAMING_CTRL)
    {
     ((USBD_AUDIO_ItfTypeDef *)pdev->pUserDataAUDIO)->MuteCtl(haudio->control.data[0]);
      haudio->control.cmd = 0U;
      haudio->control.len = 0U;
    }
  }
  return USBD_OK;
}

static uint8_t  USBD_UA3REO_EP0_TxReady (USBD_HandleTypeDef *pdev)
{
  /* Only OUT control data are processed */
  return USBD_OK;
}

static uint8_t  USBD_UA3REO_SOF (USBD_HandleTypeDef *pdev)
{
  return USBD_OK;
}

static uint8_t  USBD_UA3REO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  return USBD_OK;
}

static uint8_t  USBD_UA3REO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  return USBD_OK;
}
/**
  * @brief  USBD_CDC_GetFSCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_UA3REO_GetFSCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_UA3REO_CfgFSDesc);
  return USBD_UA3REO_CfgFSDesc;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_UA3REO_GetDeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (USBD_UA3REO_DeviceQualifierDesc);
  return USBD_UA3REO_DeviceQualifierDesc;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_DEBUG_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_DEBUG_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;
  if(fops != NULL)
  {
    pdev->pUserDataDEBUG = fops;
    ret = USBD_OK;
  }
  return ret;
}
uint8_t  USBD_CAT_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_CAT_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;
  if(fops != NULL)
  {
    pdev->pUserDataCAT = fops;
    ret = USBD_OK;
  }
  return ret;
}
uint8_t  USBD_AUDIO_RegisterInterface  (USBD_HandleTypeDef   *pdev, USBD_AUDIO_ItfTypeDef *fops)
{
  if(fops != NULL)
  {
    pdev->pUserDataAUDIO= fops;
  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_SetTxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Tx Buffer
  * @retval status
  */
uint8_t  USBD_DEBUG_SetTxBuffer  (USBD_HandleTypeDef *pdev,uint8_t  *pbuff, uint16_t length)
{
  USBD_DEBUG_HandleTypeDef   *hcdc = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;
  hcdc->TxBuffer = pbuff;
  hcdc->TxLength = length;
  return USBD_OK;
}
uint8_t  USBD_CAT_SetTxBuffer  (USBD_HandleTypeDef *pdev,uint8_t  *pbuff, uint16_t length)
{
  USBD_CAT_HandleTypeDef   *hcdc = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
  hcdc->TxBuffer = pbuff;
  hcdc->TxLength = length;
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_SetRxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Rx Buffer
  * @retval status
  */
uint8_t  USBD_DEBUG_SetRxBuffer  (USBD_HandleTypeDef *pdev, uint8_t  *pbuff)
{
  USBD_DEBUG_HandleTypeDef   *hcdc = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;
  hcdc->RxBuffer = pbuff;
  return USBD_OK;
}
uint8_t  USBD_CAT_SetRxBuffer  (USBD_HandleTypeDef *pdev, uint8_t  *pbuff)
{
  USBD_CAT_HandleTypeDef   *hcdc = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
  hcdc->RxBuffer = pbuff;
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_TransmitPacket
  *         Transmit packet on IN endpoint
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_DEBUG_TransmitPacket(USBD_HandleTypeDef *pdev)
{
  USBD_DEBUG_HandleTypeDef   *hcdc = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;
  if(pdev->pClassDataDEBUG != NULL)
  {
    if(hcdc->TxState == 0U)
    {
      /* Tx Transfer in progress */
      hcdc->TxState = 1U;
      /* Update the packet total length */
      pdev->ep_in[DEBUG_IN_EP & 0xFU].total_length = hcdc->TxLength;
      /* Transmit next packet */
      USBD_LL_Transmit(pdev, DEBUG_IN_EP, hcdc->TxBuffer, (uint16_t)hcdc->TxLength);
      return USBD_OK;
    }
    else
    {
      return USBD_BUSY;
    }
  }
  else
  {
    return USBD_FAIL;
  }
}
uint8_t  USBD_CAT_TransmitPacket(USBD_HandleTypeDef *pdev)
{
  USBD_CAT_HandleTypeDef   *hcdc = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
  if(pdev->pClassDataCAT != NULL)
  {
    if(hcdc->TxState == 0U)
    {
      hcdc->TxState = 1U;
      pdev->ep_in[CAT_IN_EP & 0xFU].total_length = hcdc->TxLength;
      USBD_LL_Transmit(pdev, CAT_IN_EP, hcdc->TxBuffer, (uint16_t)hcdc->TxLength);
      return USBD_OK;
    }
    else
    {
      return USBD_BUSY;
    }
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_CDC_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_DEBUG_ReceivePacket(USBD_HandleTypeDef *pdev)
{
  USBD_DEBUG_HandleTypeDef   *hcdc = (USBD_DEBUG_HandleTypeDef*) pdev->pClassDataDEBUG;
  /* Suspend or Resume USB Out process */
  if(pdev->pClassDataDEBUG != NULL)
  {
    /* Prepare Out endpoint to receive next packet */
    USBD_LL_PrepareReceive(pdev, DEBUG_OUT_EP, hcdc->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
uint8_t  USBD_CAT_ReceivePacket(USBD_HandleTypeDef *pdev)
{
  USBD_CAT_HandleTypeDef   *hcdc = (USBD_CAT_HandleTypeDef*) pdev->pClassDataCAT;
  /* Suspend or Resume USB Out process */
  if(pdev->pClassDataCAT != NULL)
  {
    /* Prepare Out endpoint to receive next packet */
    USBD_LL_PrepareReceive(pdev, CAT_OUT_EP, hcdc->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}


void  USBD_AUDIO_Sync (USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset)
{
  uint32_t cmd = 0U;
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassDataAUDIO;

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
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserDataAUDIO)->AudioCmd(&haudio->buffer[0],cmd,AUDIO_CMD_PLAY);
      haudio->offset = AUDIO_OFFSET_NONE;
  }
}

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassDataAUDIO;

  memset(haudio->control.data, 0, 64U);

  /* Send the current mute state */
  USBD_CtlSendData (pdev, haudio->control.data, req->wLength);
}

/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassDataAUDIO;

  if (req->wLength)
  {
    /* Prepare the reception of the buffer over EP0 */
    USBD_CtlPrepareRx (pdev,
                       haudio->control.data,
                       req->wLength);

    haudio->control.cmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
    haudio->control.len = (uint8_t)req->wLength; /* Set the request data length */
    haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
  }
}
