#include "stm32f4xx_hal.h"
#include "wm8731.h"
#include "functions.h"
#include <stdlib.h>
#include "math.h"
#include "trx_manager.h"
#include "settings.h"

uint16_t CODEC_Audio_Buffer[CODEC_AUDIO_BUFFER_SIZE] __attribute__((aligned(4)));
uint16_t CODEC_Audio_Buffer_TX[CODEC_AUDIO_BUFFER_SIZE] __attribute__((aligned(4)));

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

void stop_i2s_dma(void)
{
	HAL_I2S_DMAStop(&hi2s3);
	HAL_DMA_Abort(&hdma_i2s3_ext_rx);
	HAL_DMA_Abort(&hdma_spi3_tx);
	HAL_I2S_DeInit(&hi2s3);
	HAL_I2S_Init(&hi2s3);
	HAL_Delay(10);
	memset(CODEC_Audio_Buffer, 0xAA, sizeof(CODEC_Audio_Buffer));
	memset(CODEC_Audio_Buffer_TX, 0xAA, sizeof(CODEC_Audio_Buffer_TX));
}

void start_i2s_rx_dma(void)
{
	logToUART1_str("RX MODE\r\n");
	stop_i2s_dma();
	WM8731_RX_mode();
	if (HAL_I2S_GetState(&hi2s3) == HAL_I2S_STATE_READY)
		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_Buffer[0], sizeof(CODEC_Audio_Buffer)/2);
}

void start_i2s_tx_dma(void)
{
	logToUART1_str("TX MODE\r\n");
	stop_i2s_dma();
	WM8731_TX_mode();
	if (HAL_I2S_GetState(&hi2s3) == HAL_I2S_STATE_READY)
		HAL_I2SEx_TransmitReceive_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_Buffer[0], (uint16_t*)&CODEC_Audio_Buffer_TX[0], sizeof(CODEC_Audio_Buffer)/2);
}

void start_loopback_dma(void)
{
	logToUART1_str("LOOP MODE\r\n");
	stop_i2s_dma();
	WM8731_TXRX_mode();
	if (HAL_I2S_GetState(&hi2s3) == HAL_I2S_STATE_READY)
		HAL_I2SEx_TransmitReceive_DMA(&hi2s3, (uint16_t*)&CODEC_Audio_Buffer[0], (uint16_t*)&CODEC_Audio_Buffer[0], sizeof(CODEC_Audio_Buffer)/2);
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

void WM8731_TX_mode(void)
{
	if (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_READY) stop_i2s_dma();
	WM8731_SendI2CCommand(B8(00000100), B8(00000000)); //Left Headphone Out 
	WM8731_SendI2CCommand(B8(00000110), B8(00000000)); //Right Headphone Out
	WM8731_SendI2CCommand(B8(00001000), B8(00000101)); //R4 Analogue Audio Path Control
	WM8731_SendI2CCommand(B8(00001010), B8(00001110)); //R5  Digital Audio Path Control
	WM8731_SendI2CCommand(B8(00001100), B8(00001001)); //R6  Power Down Control
}

void WM8731_RX_mode(void)
{
	if (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_READY) stop_i2s_dma();
	WM8731_SendI2CCommand(B8(00000100), B8(01111001)); //Left Headphone Out 
	WM8731_SendI2CCommand(B8(00000110), B8(01111001)); //Right Headphone Out
	WM8731_SendI2CCommand(B8(00001000), B8(00010110)); //R4 Analogue Audio Path Control
	WM8731_SendI2CCommand(B8(00001010), B8(00000110)); //R5 Digital Audio Path Control
	WM8731_SendI2CCommand(B8(00001100), B8(00000111)); //R6  Power Down Control
}

void WM8731_TXRX_mode(void)
{
	if (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_READY) stop_i2s_dma();
	WM8731_SendI2CCommand(B8(00000100), B8(01111001)); //Left Headphone Out 
	WM8731_SendI2CCommand(B8(00000110), B8(01111001)); //Right Headphone Out
	WM8731_SendI2CCommand(B8(00001000), B8(00010101)); //R4 Analogue Audio Path Control
	WM8731_SendI2CCommand(B8(00001010), B8(00000110)); //R5  Digital Audio Path Control
	//WM8731_SendI2CCommand(B8(00001100), B8(01100001)); //R6  Power Down Control external crystal
	WM8731_SendI2CCommand(B8(00001100), B8(00000001)); //R6  Power Down Control, internal crystal
}

void WM8731_Init(void)
{
	logToUART1_str("WM8731 ");
	WM8731_SendI2CCommand(B8(00011110),B8(00000000)); //R15 Reset Chip
	WM8731_SendI2CCommand(B8(00000000), B8(10000000)); //Left Line In
	WM8731_SendI2CCommand(B8(00000010), B8(10000000)); //Right Line In 
	WM8731_SendI2CCommand(B8(00001110), B8(00000010)); //R7  Digital Audio Interface Format, Codec Slave, I2S Format, MSB-First left-1 justified , 16bits
	//WM8731_SendI2CCommand(B8(00001110), B8(01000000)); //R7  Digital Audio Interface Format, Codec Master, I2S Format, MSB-First left-1 justified , 16bits
	WM8731_SendI2CCommand(B8(00010000), B8(00000001)); //R8  Sampling Control normal mode, 250fs, SR=0 (MCLK@12Mhz, fs=48kHz)) 0 0000
	WM8731_RX_mode();
	WM8731_SendI2CCommand(B8(00010010), B8(00000001)); //R9  reactivate digital audio interface

	logToUART1_str(" Inited\r\n");
}
