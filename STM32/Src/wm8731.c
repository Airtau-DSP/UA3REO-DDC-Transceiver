#include "stm32f4xx_hal.h"
#include "wm8731.h"
#include "functions.h"
#include <stdlib.h>
#include "math.h"
#include "trx_manager.h"
#include "settings.h"

uint16_t CODEC_Audio_Buffer[CODEC_AUDIO_BUFFER_SIZE] = { 0 };
uint16_t CODEC_Audio_Buffer_TX[CODEC_AUDIO_BUFFER_SIZE] = { 0 };

uint8_t WM8731_SampleMode = 48;
uint32_t WM8731_DMA_samples = 0;
bool WM8731_DMA_state = true; //true - compleate ; false - half

void start_i2s_dma(void)
{
	if (TRX_ptt)
	{
		start_i2s_tx_dma();
	}
	else
	{
		if (TRX.Loopback)
			start_loopback_dma();
		else
			start_i2s_rx_dma();
	}
}

void start_i2s_rx_dma(void)
{
	if (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_READY) HAL_I2S_DMAStop(&hi2s3);
	memchr((uint16_t*)&CODEC_Audio_Buffer[0], 0, CODEC_AUDIO_BUFFER_SIZE);
	if (HAL_I2S_GetState(&hi2s3) == HAL_I2S_STATE_READY)
		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_Buffer[0], CODEC_AUDIO_BUFFER_SIZE);
}

void start_i2s_tx_dma(void)
{
	if (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_READY) HAL_I2S_DMAStop(&hi2s3);
	memchr((uint16_t*)&CODEC_Audio_Buffer[0], 0, CODEC_AUDIO_BUFFER_SIZE);
	memchr((uint16_t*)&CODEC_Audio_Buffer_TX[0], 0, CODEC_AUDIO_BUFFER_SIZE);
	if (HAL_I2S_GetState(&hi2s3) == HAL_I2S_STATE_READY)
		HAL_I2SEx_TransmitReceive_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_Buffer[0], (uint16_t*)&CODEC_Audio_Buffer_TX[0], CODEC_AUDIO_BUFFER_SIZE);
}

void start_loopback_dma(void)
{
	if (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_READY) HAL_I2S_DMAStop(&hi2s3);
	memchr((uint16_t*)&CODEC_Audio_Buffer[0], 0, CODEC_AUDIO_BUFFER_SIZE);
	if (HAL_I2S_GetState(&hi2s3) == HAL_I2S_STATE_READY)
		HAL_I2SEx_TransmitReceive_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_Buffer[0], (uint16_t*)&CODEC_Audio_Buffer[0], CODEC_AUDIO_BUFFER_SIZE);
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if (hi2s->Instance == SPI3)
	{
		WM8731_DMA_state = true;
		Processor_NeedBuffer = true;
		WM8731_DMA_samples += FPGA_AUDIO_BUFFER_SIZE;
	}
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if (hi2s->Instance == SPI3)
	{
		WM8731_DMA_state = false;
		Processor_NeedBuffer = true;
		WM8731_DMA_samples += FPGA_AUDIO_BUFFER_SIZE;
	}
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if (hi2s->Instance == SPI3)
	{
		WM8731_DMA_state = true;
		Processor_NeedBuffer = true;
		WM8731_DMA_samples += FPGA_AUDIO_BUFFER_SIZE;
	}
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if (hi2s->Instance == SPI3)
	{
		WM8731_DMA_state = false;
		Processor_NeedBuffer = true;
		WM8731_DMA_samples += FPGA_AUDIO_BUFFER_SIZE;
	}
}

void WM8731_switchToActualSampleRate(int32_t rate)
{
	uint8_t mode = 48;
	uint32_t min_diff = 9999999;
	//if(abs(96000-rate)<min_diff) { mode=96; min_diff=abs(96000-rate); }
	if (abs(48000 - rate) < min_diff) { mode = 48; min_diff = abs(48000 - rate); }
	if (abs(44000 - rate) < min_diff) { mode = 44; min_diff = abs(44000 - rate); }
	if (abs(32000 - rate) < min_diff) { mode = 32; min_diff = abs(32000 - rate); }
	if (abs(8000 - rate) < min_diff) { mode = 8; min_diff = abs(8000 - rate); }
	if (WM8731_SampleMode != mode)
	{
		logToUART1_str("Change WM8731 SampleMode From ");
		logToUART1_numinline(WM8731_SampleMode);
		logToUART1_str(" To ");
		logToUART1_numinline(mode);
		logToUART1_str("\r\n");

		//HAL_I2S_DMAStop(&hi2s3);
		//HAL_I2S_DeInit(&hi2s3);	

		switch (mode)
		{
		case 96:
			WM8731_SendI2CCommand(B8(00010000), B8(00011101)); //R8  Sampling Control normal mode, 250fs, SR=0 (MCLK@12Mhz, fs=96kHz)) 0 0111
			hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_96K;
			break;
		case 48:
			WM8731_SendI2CCommand(B8(00010000), B8(00000001)); //R8  Sampling Control normal mode, 250fs, SR=0 (MCLK@12Mhz, fs=48kHz)) 0 0000
			hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_48K;
			break;
		case 44:
			WM8731_SendI2CCommand(B8(00010000), B8(00100011)); //R8  Sampling Control normal mode, 250fs, SR=0 (MCLK@12Mhz, fs=48kHz)) 1 1000
			hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;
			break;
		case 32:
			WM8731_SendI2CCommand(B8(00010000), B8(00011001)); //R8  Sampling Control normal mode, 250fs, SR=0 (MCLK@12Mhz, fs=48kHz)) 0 0110
			hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_32K;
			break;
		case 8:
			WM8731_SendI2CCommand(B8(00010000), B8(00001101)); //R8  Sampling Control normal mode, 250fs, SR=0 (MCLK@12Mhz, fs=48kHz)) 0 0011
			hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_8K;
			break;
		}

		//HAL_I2S_Init(&hi2s3);
		//if(CODEC_Audio_OUT_ActiveBuffer==0) HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_OUT_Buffer_A[0], CODEC_AUDIO_BUFFER_SIZE);
		//if(CODEC_Audio_OUT_ActiveBuffer==1) HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_OUT_Buffer_B[0], CODEC_AUDIO_BUFFER_SIZE);

		WM8731_SampleMode = mode;
	}
}

void WM8731_SendI2CCommand(uint8_t reg, uint8_t value)
{
	uint8_t st = 2;
	while (st != 0)
	{
		i2c_begin();
		i2c_beginTransmission_u8(B8(0011010)); //I2C_ADDRESS_WM8731 00110100
		i2c_write_u8(reg); // MSB
		i2c_write_u8(value); // MSB
		st = i2c_endTransmission();
		HAL_Delay(1);
		//logToUART1_numinline(st);
	}
	//logToUART1_str(".");
}

void WM8731_Init(void)
{
	logToUART1_str("WM8731 ");
	//WM8731_SendI2CCommand(B8(00011110),B8(00000000)); //R15 Reset Chip

	WM8731_SendI2CCommand(B8(00000000), B8(10000000)); //Left Line In
	WM8731_SendI2CCommand(B8(00000010), B8(10000000)); //Right Line In 
	WM8731_SendI2CCommand(B8(00000100), B8(01111001)); //Left Headphone Out 
	WM8731_SendI2CCommand(B8(00000110), B8(01111001)); //Right Headphone Out 
	WM8731_SendI2CCommand(B8(00001000), B8(00010100)); //R4 Analogue Audio Path Control dacsel, micboost=off
	WM8731_SendI2CCommand(B8(00001010), B8(00000110)); //R5  Digital Audio Path Control dacmu off
	WM8731_SendI2CCommand(B8(00001100), B8(01100001)); //R6  Power Down Control dac on, out on
	WM8731_SendI2CCommand(B8(00001110), B8(00010010)); //R7  Digital Audio Interface Format slave, I2S Format, MSB-First left-1 justified , 16bits
	WM8731_switchToActualSampleRate(48000);
	//WM8731_SendI2CCommand(B8(00010010),B8(00000000)); //R9  deactivate digital audio interface
	WM8731_SendI2CCommand(B8(00010010), B8(00000001)); //R9  reactivate digital audio interface

	logToUART1_str(" Inited\r\n");
}
