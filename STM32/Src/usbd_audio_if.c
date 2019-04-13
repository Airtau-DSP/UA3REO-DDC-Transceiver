/* Includes ------------------------------------------------------------------*/
#include "usbd_audio_if.h"
#include "functions.h"
#include "wm8731.h"
#include "trx_manager.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t AUDIO_Init_FS(uint32_t options);
static int8_t AUDIO_DeInit_FS(uint32_t options);

int16_t USB_AUDIO_rx_buffer_a[(USB_AUDIO_RX_BUFFER_SIZE / 2)] = { 0 };
int16_t USB_AUDIO_rx_buffer_b[(USB_AUDIO_RX_BUFFER_SIZE / 2)] = { 0 };

int16_t USB_AUDIO_tx_buffer[(USB_AUDIO_TX_BUFFER_SIZE/2)] = { 0 };
//on FPGA BUFFER 192*4=768 half words, AUDIO_TX_BUFFER_SIZE  (8 bit) is 3072 bytes and 1536 half words


bool USB_AUDIO_current_rx_buffer = false; // a-false b-true
bool USB_AUDIO_need_rx_buffer = false; // a-false b-true

USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops_FS =
{
  AUDIO_Init_FS,
  AUDIO_DeInit_FS,
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the AUDIO media low layer over USB FS IP
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @param  Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param  options: Reserved for future use
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */

static int8_t AUDIO_Init_FS(uint32_t options)
{
	USBD_AUDIO_HandleTypeDef   *haudio = (USBD_AUDIO_HandleTypeDef*) hUsbDeviceFS.pClassDataAUDIO;
	haudio->TxBuffer=(uint8_t*)&USB_AUDIO_tx_buffer;
	haudio->TxBufferIndex=0;
	USBD_AUDIO_StartTransmit(&hUsbDeviceFS);
	USBD_AUDIO_StartReceive(&hUsbDeviceFS);
	return (USBD_OK);
}

int16_t USB_AUDIO_GetTXBufferIndex_FS(void)
{
	USBD_AUDIO_HandleTypeDef   *haudio = (USBD_AUDIO_HandleTypeDef*) hUsbDeviceFS.pClassDataAUDIO;
	return haudio->TxBufferIndex;
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
