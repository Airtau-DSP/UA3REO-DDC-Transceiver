#include "stm32f4xx_hal.h"
#include "fpga.h"
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "audio_processor.h"
#include "settings.h"
#include "wm8731.h"

uint16_t FPGA_fpgadata_in_tmp16 = 0;
int16_t FPGA_fpgadata_out_tmp16 = 0;
int16_t FPGA_fpgadata_in_inttmp16 = 0;
uint8_t FPGA_fpgadata_in_tmp8 = 0;
uint8_t FPGA_fpgadata_out_tmp8 = 0;

bool FPGA_busy = false;
GPIO_InitTypeDef FPGA_GPIO_InitStruct;

uint32_t FPGA_samples = 0;

float32_t FPGA_Audio_Buffer_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
uint16_t FPGA_Audio_Buffer_Index = 0;
bool FPGA_Audio_Buffer_State = true; //true - compleate ; false - half

bool FPGA_NeedSendParams = false;
bool FPGA_NeedGetParams = false;
bool FPGA_Buffer_underrun = false;

void FPGA_Init(void)
{
	//шина данных STM32-FPGA
	FPGA_testbus();
	FPGA_start_audio_clock();
}

void FPGA_start_audio_clock(void) //запуск PLL для I2S и кодека, при включенном тактовом не программируется i2c
{
	FPGA_busy = true;
	//STAGE 1
	//out
	FPGA_writePacket(5);
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);
	FPGA_busy = false;
}

void FPGA_stop_audio_clock(void) //остановка PLL для I2S и кодека, при включенном тактовом не программируется i2c
{
	FPGA_busy = true;
	//STAGE 1
	//out
	FPGA_writePacket(6);
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);
	FPGA_busy = false;
}

void FPGA_testbus(void) //проверка целостности шины данных STM32-FPGA
{
	logToUART1_str("FPGA Bus Test ");

	FPGA_busy = true;
	//обмен данными

	//STAGE 1
	//out
	FPGA_writePacket(B8(00001010));
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

	//STAGE 2
	//out
	FPGA_writePacket(B8(00000001));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00000001))
	{
		logToUART1_str("ERROR 0 PIN\r\n");
		logToUART1_num(FPGA_fpgadata_in_tmp8);
		return;
	}
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 3
	//out
	FPGA_writePacket(B8(00000010));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00000010))
	{
		logToUART1_str("ERROR 1 PIN\r\n");
		logToUART1_num(FPGA_fpgadata_in_tmp8);
		return;
	}
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 4
	//out
	FPGA_writePacket(B8(00000100));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00000100))
	{
		logToUART1_str("ERROR 2 PIN\r\n");
		logToUART1_num(FPGA_fpgadata_in_tmp8);
		return;
	}
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 5
	//out
	FPGA_writePacket(B8(00001000));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00001000))
	{
		logToUART1_str("ERROR 3 PIN\r\n");
		logToUART1_num(FPGA_fpgadata_in_tmp8);
		return;
	}
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	FPGA_busy = false;

	logToUART1_str("OK\r\n");
}

void FPGA_fpgadata_stuffclock(void)
{
	FPGA_busy = true;
	//обмен данными

	//STAGE 1
	//out
	FPGA_fpgadata_out_tmp8 = 0;
	if (FPGA_NeedSendParams) FPGA_fpgadata_out_tmp8 = 1;
	else if (FPGA_NeedGetParams)  FPGA_fpgadata_out_tmp8 = 2;

	if (FPGA_fpgadata_out_tmp8 != 0)
	{
		FPGA_writePacket(FPGA_fpgadata_out_tmp8);
		//clock
		GPIOC->BSRR = FPGA_SYNC_Pin;
		HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
		//in
		//clock
		GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

		if (FPGA_NeedSendParams) { FPGA_fpgadata_sendparam(); FPGA_NeedSendParams = false; }
		else if (FPGA_NeedGetParams) { FPGA_fpgadata_getparam(); FPGA_NeedGetParams = false; }
	}
	FPGA_busy = false;
}

void FPGA_fpgadata_iqclock(void)
{
	FPGA_busy = true;
	//обмен данными

	//STAGE 1
	//out
	if (TRX_ptt || TRX_tune) FPGA_fpgadata_out_tmp8 = 3;
	else FPGA_fpgadata_out_tmp8 = 4;

	FPGA_writePacket(FPGA_fpgadata_out_tmp8);
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

	if (TRX_ptt || TRX_tune) FPGA_fpgadata_sendiq();
	else FPGA_fpgadata_getiq();

	FPGA_busy = false;
}

void FPGA_fpgadata_sendparam(void)
{
	uint32_t TRX_freq_phrase = getPhraseFromFrequency(TRX.Freq);
	//STAGE 2
	//out PTT+PREAMP
	FPGA_fpgadata_out_tmp8 = 0;
	bitWrite(FPGA_fpgadata_out_tmp8, 3, TRX_ptt || TRX_tune);
	if (!TRX_ptt && !TRX_tune) bitWrite(FPGA_fpgadata_out_tmp8, 2, TRX.Preamp_UHF);
	FPGA_writePacket(FPGA_fpgadata_out_tmp8);
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

	//STAGE 3
	//out FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 20)) >> 20));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 4
	//out FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 16)) >> 16));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 5
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 12)) >> 12));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 6
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 8)) >> 8));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 7
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 4)) >> 4));
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 8
	//OUT FREQ
	FPGA_writePacket(TRX_freq_phrase & 0XF);
	//clock
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
}

void FPGA_fpgadata_getparam(void)
{
	//STAGE 2
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	TRX_ADC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 0);
	TRX_DAC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 1);
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
}

void FPGA_fpgadata_getiq(void)
{
	FPGA_samples++;

	//STAGE 2
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XF) << 12;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 3
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 8;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 4
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 4;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 5
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF);
	FPGA_fpgadata_in_inttmp16 = FPGA_fpgadata_in_tmp16;
	if (FFTInputBufferInProgress) // A buffer in progress
	{
		FFTInput_A[FFT_buff_index + 1] = (int16_t)FPGA_fpgadata_in_inttmp16 / 32767.0f;
		FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] = FFTInput_A[FFT_buff_index + 1];
	}
	else // B buffer in progress
	{
		FFTInput_B[FFT_buff_index + 1] = (int16_t)FPGA_fpgadata_in_inttmp16 / 32767.0f;
		FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] = FFTInput_B[FFT_buff_index + 1];
	}
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 6
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XF) << 12;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 7
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 8;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 8
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 4;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 9
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF);
	FPGA_fpgadata_in_inttmp16 = FPGA_fpgadata_in_tmp16;
	if (FFTInputBufferInProgress) // A buffer in progress
	{
		FFTInput_A[FFT_buff_index] = (int16_t)FPGA_fpgadata_in_inttmp16 / 32767.0f;
		FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] = FFTInput_A[FFT_buff_index];
	}
	else // B buffer in progress
	{
		FFTInput_B[FFT_buff_index] = (int16_t)FPGA_fpgadata_in_inttmp16 / 32767.0f;
		FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] = FFTInput_B[FFT_buff_index];
	}
	FPGA_Audio_Buffer_Index++;
	FFT_buff_index += 2;
	if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE) FPGA_Audio_Buffer_Index = 0;
	if (FFT_buff_index == FFT_SIZE * 2)
	{
		FFT_buff_index = 0;
		FFTInputBufferInProgress = !FFTInputBufferInProgress;
	}
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
}

void FPGA_fpgadata_sendiq(void)
{
	FPGA_samples++;
	
	//STAGE 2 out Q
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] * MAX_TX_AMPLITUDE;
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 12);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 3
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 8);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 4
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 4);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 5
	FPGA_writePacket(FPGA_fpgadata_out_tmp16);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 6 out I
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] * MAX_TX_AMPLITUDE;
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 12);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 7
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 8);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 8
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 4);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 9
	FPGA_writePacket(FPGA_fpgadata_out_tmp16);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	FPGA_Audio_Buffer_Index++;
	if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE)
	{
		if(Processor_NeedBuffer) FPGA_Buffer_underrun=true;
		FPGA_Audio_Buffer_Index = 0;
		FPGA_Audio_Buffer_State = true;
		Processor_NeedBuffer = true;
	}
	else if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE / 2)
	{
		if(Processor_NeedBuffer) FPGA_Buffer_underrun=true;
		FPGA_Audio_Buffer_State = false;
		Processor_NeedBuffer = true;
	}
}

inline uint8_t FPGA_readPacket(void)
{
	return (((FPGA_IN_D3_GPIO_Port->IDR & FPGA_IN_D3_Pin) == FPGA_IN_D3_Pin) << 3) | (((FPGA_IN_D2_GPIO_Port->IDR & FPGA_IN_D2_Pin) == FPGA_IN_D2_Pin) << 2) | (((FPGA_IN_D1_GPIO_Port->IDR & FPGA_IN_D1_Pin) == FPGA_IN_D1_Pin) << 1) | (((FPGA_IN_D0_GPIO_Port->IDR & FPGA_IN_D0_Pin) == FPGA_IN_D0_Pin));
}

inline void FPGA_writePacket(uint8_t packet)
{
	FPGA_OUT_D0_GPIO_Port->BSRR = (bitRead(packet, 0) << 4 & FPGA_OUT_D0_Pin) | (bitRead(packet, 1) << 5 & FPGA_OUT_D1_Pin) | (bitRead(packet, 2) << 6 & FPGA_OUT_D2_Pin) | (bitRead(packet, 3) << 7 & FPGA_OUT_D3_Pin) | ((uint32_t)FPGA_OUT_D3_Pin << 16U) | ((uint32_t)FPGA_OUT_D2_Pin << 16U) | ((uint32_t)FPGA_OUT_D1_Pin << 16U) | ((uint32_t)FPGA_OUT_D0_Pin << 16U);
}
