#include "functions.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include "fpga.h"
#include "trx_manager.h"
#include "usbd_debug_if.h"
#include "usbd_cat_if.h"

void dma_memcpy32(uint32_t dest, uint32_t src, uint32_t len)
{
	HAL_DMA_Start(&hdma_memtomem_dma2_stream2, src, dest, len);
	HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream2, HAL_DMA_FULL_TRANSFER, 10);
}

void readHalfFromCircleBuffer32(uint32_t *source, uint32_t *dest, uint32_t index, uint32_t length)
{
	uint16_t halflen = length / 2;
	if (index >= halflen)
	{
		dma_memcpy32((uint32_t)&dest[0], (uint32_t)&source[index - halflen], halflen);
	}
	else
	{
		uint16_t prev_part = halflen - index;
		dma_memcpy32((uint32_t)&dest[0], (uint32_t)&source[length - prev_part], prev_part);
		dma_memcpy32((uint32_t)&dest[prev_part], (uint32_t)&source[0], (halflen - prev_part));
	}
}

void readHalfFromCircleUSBBuffer(int16_t *source, int32_t *dest, uint16_t index, uint16_t length)
{
	uint16_t halflen = length / 2;
	uint16_t readed_index=0;
	uint16_t start_index=0;
	if(index >= halflen)
	{
		for(uint16_t i=(index - halflen);i<index;i++)
		{
			dest[readed_index]=source[i];
			readed_index++;
		}
	}
	else
	{
		uint16_t prev_part = halflen - index;
		for(uint16_t i=(length - prev_part);i<length;i++)
		{
			dest[readed_index]=source[i];
			readed_index++;
		}
		for(uint16_t i=0;i<(halflen - prev_part);i++)
		{
			dest[readed_index]=source[i];
			readed_index++;
		}
	}
}

void sendToDebug_str(char* data)
{
	DEBUG_Transmit_FIFO((uint8_t*)data, strlen(data));
	HAL_UART_Transmit(&huart1, (uint8_t*)data, strlen(data), 1000);
}

void sendToDebug_str2(char* data1,char* data2)
{
	sendToDebug_str(data1);
	sendToDebug_str(data2);
}

void sendToDebug_str3(char* data1,char* data2,char* data3)
{
	sendToDebug_str(data1);
	sendToDebug_str(data2);
	sendToDebug_str(data3);
}

void sendToDebug_newline(void)
{
	sendToDebug_str("\r\n");
}

void sendToDebug_uint8(uint8_t data, bool _inline)
{
	char tmp[50] = "";
	if(_inline)
		sprintf(tmp, "%d", data);
	else
		sprintf(tmp, "%d\r\n", data);
	sendToDebug_str(tmp);
}

void sendToDebug_uint16(uint16_t data, bool _inline)
{
	char tmp[50] = "";
	if(_inline)
		sprintf(tmp, "%d", data);
	else
		sprintf(tmp, "%d\r\n", data);
	sendToDebug_str(tmp);
}
void sendToDebug_uint32(uint32_t data, bool _inline)
{
	char tmp[50] = "";
	if(_inline)
		sprintf(tmp, "%d", data);
	else
		sprintf(tmp, "%d\r\n", data);
	sendToDebug_str(tmp);
}
void sendToDebug_int16(int16_t data, bool _inline)
{
	char tmp[50] = "";
	if(_inline)
		sprintf(tmp, "%d", data);
	else
		sprintf(tmp, "%d\r\n", data);
	sendToDebug_str(tmp);
}
void sendToDebug_int32(int32_t data, bool _inline)
{
	char tmp[50] = "";
	if(_inline)
		sprintf(tmp, "%d", data);
	else
		sprintf(tmp, "%d\r\n", data);
	sendToDebug_str(tmp);
}

void sendToDebug_float32(float32_t data, bool _inline)
{
	char tmp[50] = "";
	if(_inline)
		sprintf(tmp, "%f", data);
	else
		sprintf(tmp, "%f\r\n", data);
	sendToDebug_str(tmp);
}

void delay_us(uint32_t us)
{
	int32_t us_count_tick = us * (SystemCoreClock / 1000000);
	//разрешаем использовать счётчик
	SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	//обнуляем значение счётного регистра
	DWT_CYCCNT = 0;
	//запускаем счётчик
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;
	while (DWT_CYCCNT < us_count_tick);
	//останавливаем счётчик
	DWT_CONTROL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t getFrequencyFromPhrase(uint32_t phrase) //высчитываем фазу частоты для FPGA
{
	uint32_t res = 0;
	res = ceil(((double)phrase / 4194304) * ADCDAC_CLOCK / 100) * 100; //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;
	return res;
}

uint32_t getPhraseFromFrequency(uint32_t freq) //высчитываем частоту из фразы ля FPGA
{
	bool inverted = false;
	uint32_t res = 0;
	uint32_t _freq = freq;
	if (_freq > ADCDAC_CLOCK / 2) //Go Nyquist
	{
		while (_freq > ADCDAC_CLOCK / 2)
		{
			_freq -= ADCDAC_CLOCK / 2;
			inverted = !inverted;
		}
		if (inverted)
		{
			_freq = ADCDAC_CLOCK / 2 - _freq;
		}
	}
	TRX_IQ_swap=inverted;
	res = round(((double)_freq / ADCDAC_CLOCK) * 4194304); //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;
	return res;
}

uint32_t hexStringToInt(char* in) //преобразование строки шестнадцатеричного числа в число
{
	char* res;
	strcpy(res, "0");
	if (strlen(in) % 2)
	{
		strcat(res, in);
		in = res;
	}
	return strtol(in, NULL, 16);
}

void addSymbols(char* dest, char* str, uint8_t length, char* symbol, bool toEnd) //добавляем нули
{
	char res[50] = "";
	strcpy(res, str);
	while (strlen(res) < length)
	{
		if (toEnd)
			strcat(res, symbol);
		else
		{
			char tmp[50] = "";
			strcat(tmp, symbol);
			strcat(tmp, res);
			strcpy(res, tmp);
		}
	}
	strcpy(dest, res);
}

float log10f_fast(float X) {
	float Y, F;
	int E;
	F = frexpf(fabsf(X), &E);
	Y = 1.23149591368684f;
	Y *= F;
	Y += -4.11852516267426f;
	Y *= F;
	Y += 6.02197014179219f;
	Y *= F;
	Y += -3.13396450166353f;
	Y += E;
	return(Y * 0.3010299956639812f);
}
