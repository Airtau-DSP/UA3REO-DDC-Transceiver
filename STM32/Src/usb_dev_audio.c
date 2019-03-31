#include "usbd_audio.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "functions.h"

static int8_t  ua3reo_dev_audio_Init(void* itf, uint32_t  AudioFreq, uint32_t Volume, uint32_t options);
static int8_t  ua3reo_dev_audio_DeInit(void* itf, uint32_t options);
static int8_t  ua3reo_dev_audio_AudioCmd(void* itf, uint8_t* pbuf, uint32_t size, uint8_t cmd);
static int8_t  ua3reo_dev_audio_VolumeCtl(void* itf, uint8_t vol);
static int8_t  ua3reo_dev_audio_MuteCtl(void* itf, uint8_t cmd);
static int8_t  ua3reo_dev_audio_PeriodicTC(void* itf, uint8_t cmd);
static int8_t  ua3reo_dev_audio_GetState(void* itf);

static const USBD_AUDIO_AppType ua3reo_dev_audio_app =
{
	.Name = "UA3REO AUDIO Interface",
	.Init = ua3reo_dev_audio_Init,
	  .DeInit = ua3reo_dev_audio_DeInit,
	.AudioCmd = ua3reo_dev_audio_AudioCmd,
	.VolumeCtl = ua3reo_dev_audio_VolumeCtl,
		.MuteCtl = ua3reo_dev_audio_MuteCtl,
		.PeriodicTC = ua3reo_dev_audio_PeriodicTC,
		.GetState = ua3reo_dev_audio_GetState,
};

USBD_AUDIO_IfHandleType _ua3reo_dev_audio_if = {
	.App = &ua3reo_dev_audio_app,
	.Base.AltCount = 1,
}, *const ua3reo_dev_audio_if = &_ua3reo_dev_audio_if;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  ua3reo_dev_audio_Init
  *         Initializes the AUDIO media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t ua3reo_dev_audio_Init(void* itf, uint32_t  AudioFreq, uint32_t Volume, uint32_t options)
{
	/*
	   Add your initialization code here
	*/
	return (0);
}

/**
  * @brief  ua3reo_dev_audio_DeInit
  *         DeInitializes the AUDIO media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t ua3reo_dev_audio_DeInit(void* itf, uint32_t options)
{
	/*
	   Add your deinitialization code here
	*/
	return (0);
}


/**
  * @brief  ua3reo_dev_audio_AudioCmd
  *         AUDIO command handler
  * @param  Buf: Buffer of data to be sent
  * @param  size: Number of data to be sent (in bytes)
  * @param  cmd: command opcode
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t ua3reo_dev_audio_AudioCmd(void* itf, uint8_t* pbuf, uint32_t size, uint8_t cmd)
{

	return (0);
}

/**
  * @brief  ua3reo_dev_audio_VolumeCtl
  * @param  vol: volume level (0..100)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t ua3reo_dev_audio_VolumeCtl(void* itf, uint8_t vol)
{

	return (0);
}

/**
  * @brief  ua3reo_dev_audio_MuteCtl
  * @param  cmd: vmute command
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t ua3reo_dev_audio_MuteCtl(void* itf, uint8_t cmd)
{

	return (0);
}

/**
  * @brief  ua3reo_dev_audio_PeriodicTC
  * @param  cmd
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t ua3reo_dev_audio_PeriodicTC(void* itf, uint8_t cmd)
{

	return (0);
}

/**
  * @brief  ua3reo_dev_audio_GetState
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t ua3reo_dev_audio_GetState(void* itf)
{

	return (0);
}
