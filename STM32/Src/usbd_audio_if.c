/* Includes ------------------------------------------------------------------*/
#include "usbd_audio_if.h"
#include "functions.h"
#include "wm8731.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t AUDIO_Init_FS(uint32_t AudioFreq, uint32_t Volume, uint32_t options);
static int8_t AUDIO_DeInit_FS(uint32_t options);
static int8_t AUDIO_AudioCmd_FS(uint8_t* pbuf, uint32_t size, uint8_t cmd);
static int8_t AUDIO_VolumeCtl_FS(uint8_t vol);
static int8_t AUDIO_MuteCtl_FS(uint8_t cmd);
static int8_t AUDIO_PeriodicTC_FS(uint8_t cmd);
static int8_t AUDIO_GetState_FS(void);

static uint16_t rx_audio_buffer_a[(AUDIO_OUT_PACKET/2)]={0};
static uint16_t rx_audio_buffer_b[(AUDIO_OUT_PACKET/2)]={0};
static bool current_rx_audio_buffer=false; // a-false b-true
static bool need_rx_audio_buffer=false;

USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops_FS =
{
  AUDIO_Init_FS,
  AUDIO_DeInit_FS,
  AUDIO_AudioCmd_FS,
  AUDIO_VolumeCtl_FS,
  AUDIO_MuteCtl_FS,
  AUDIO_PeriodicTC_FS,
  AUDIO_GetState_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the AUDIO media low layer over USB FS IP
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @param  Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param  options: Reserved for future use 
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */

static int8_t AUDIO_Init_FS(uint32_t AudioFreq, uint32_t Volume, uint32_t options)
{
	USBD_AUDIO_StartTransmit(&hUsbDeviceFS);
  return (USBD_OK);
}

void USBAUDIO_PrepareRxBuffer_FS(void)
{
	if(!need_rx_audio_buffer) return;
	uint16_t dma_index=CODEC_AUDIO_BUFFER_SIZE-__HAL_DMA_GET_COUNTER(hi2s3.hdmarx)/2;
	if((dma_index%2)==1) dma_index++;
	for(uint8_t i=0;i<(AUDIO_OUT_PACKET/2);i++)
	{
		if(current_rx_audio_buffer)
			rx_audio_buffer_a[i]=(uint16_t)CODEC_Audio_Buffer_RX[dma_index];
		else
			rx_audio_buffer_b[i]=(uint16_t)CODEC_Audio_Buffer_RX[dma_index];
		if(dma_index==0)
			dma_index=CODEC_AUDIO_BUFFER_SIZE;
		else
			dma_index++;
	}
	need_rx_audio_buffer=false;
}

void AUDIO_GetRxBuffer_FS(USBD_HandleTypeDef *pdev)
{
	current_rx_audio_buffer=!current_rx_audio_buffer;
	if(current_rx_audio_buffer)
		USBD_AUDIO_SetTxBuffer(pdev, (uint8_t*)&rx_audio_buffer_b, AUDIO_OUT_PACKET);
	else
		USBD_AUDIO_SetTxBuffer(pdev, (uint8_t*)&rx_audio_buffer_a, AUDIO_OUT_PACKET);
	need_rx_audio_buffer=true;
}

/**
  * @brief  De-Initializes the AUDIO media low layer
  * @param  options: Reserved for future use
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_DeInit_FS(uint32_t options)
{
  /* USER CODE BEGIN 1 */
  return (USBD_OK);
  /* USER CODE END 1 */
}

/**
  * @brief  Handles AUDIO command.
  * @param  pbuf: Pointer to buffer of data to be sent
  * @param  size: Number of data to be sent (in bytes)
  * @param  cmd: Command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_AudioCmd_FS(uint8_t* pbuf, uint32_t size, uint8_t cmd)
{
  /* USER CODE BEGIN 2 */
  switch(cmd)
  {
    case AUDIO_CMD_START:
    break;

    case AUDIO_CMD_PLAY:
    break;	
  }
  return (USBD_OK);
  /* USER CODE END 2 */
}

/**
  * @brief  Controls AUDIO Volume.
  * @param  vol: volume level (0..100)
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_VolumeCtl_FS(uint8_t vol)
{
  /* USER CODE BEGIN 3 */
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  Controls AUDIO Mute.
  * @param  cmd: command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_MuteCtl_FS(uint8_t cmd)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  AUDIO_PeriodicT_FS
  * @param  cmd: Command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_PeriodicTC_FS(uint8_t cmd)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Gets AUDIO State.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_GetState_FS(void)
{
  /* USER CODE BEGIN 6 */
  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  Manages the DMA full transfer complete event.
  * @retval None
  */
void TransferComplete_CallBack_FS(void)
{
  /* USER CODE BEGIN 7 */
  USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_FULL);
  /* USER CODE END 7 */
}

/**
  * @brief  Manages the DMA Half transfer complete event.
  * @retval None
  */
void HalfTransfer_CallBack_FS(void)
{
  /* USER CODE BEGIN 8 */
  USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_HALF);
  /* USER CODE END 8 */
}
