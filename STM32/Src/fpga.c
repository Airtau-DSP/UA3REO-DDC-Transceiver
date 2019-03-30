#include "stm32f4xx_hal.h"
#include "main.h"
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
float32_t FPGA_Audio_SendBuffer_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_SendBuffer_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
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
	FPGA_clockRise();
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
	FPGA_clockRise();
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);
	FPGA_busy = false;
}

void FPGA_testbus(void) //проверка целостности шины данных STM32-FPGA
{
	sendToDebug_str("FPGA Bus Test ");

	FPGA_busy = true;
	//обмен данными

	//STAGE 1
	//out
	FPGA_writePacket(B8(00001010));
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	FPGA_clockRise();
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

	//STAGE 2
	//out
	FPGA_writePacket(B8(00000001));
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00000001))
	{
		sendToDebug_str("ERROR 0 PIN\r\n");
		sendToDebug_uint8(FPGA_fpgadata_in_tmp8,false);
		LCD_showError("FPGA bus PIN0 error");
		return;
	}
	//clock
	FPGA_clockFall();

	//STAGE 3
	//out
	FPGA_writePacket(B8(00000010));
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00000010))
	{
		sendToDebug_str("ERROR 1 PIN\r\n");
		sendToDebug_uint8(FPGA_fpgadata_in_tmp8,false);
		LCD_showError("FPGA bus PIN1 error");
		return;
	}
	//clock
	FPGA_clockFall();

	//STAGE 4
	//out
	FPGA_writePacket(B8(00000100));
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00000100))
	{
		sendToDebug_str("ERROR 2 PIN\r\n");
		sendToDebug_uint8(FPGA_fpgadata_in_tmp8,false);
		LCD_showError("FPGA bus PIN2 error");
		return;
	}
	//clock
	FPGA_clockFall();

	//STAGE 5
	//out
	FPGA_writePacket(B8(00001000));
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	if (FPGA_fpgadata_in_tmp8 != B8(00001000))
	{
		sendToDebug_str("ERROR 3 PIN\r\n");
		sendToDebug_uint8(FPGA_fpgadata_in_tmp8,false);
		LCD_showError("FPGA bus PIN3 error");
		return;
	}
	//clock
	FPGA_clockFall();

	FPGA_busy = false;

	sendToDebug_str("OK\r\n");
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
		FPGA_clockRise();
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
	if (TRX_on_TX() && TRX_getMode()!=TRX_MODE_LOOPBACK) FPGA_fpgadata_out_tmp8 = 3;
	else FPGA_fpgadata_out_tmp8 = 4;

	FPGA_writePacket(FPGA_fpgadata_out_tmp8);
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	FPGA_clockRise();
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

	if (TRX_on_TX() && TRX_getMode()!=TRX_MODE_LOOPBACK) FPGA_fpgadata_sendiq();
	else FPGA_fpgadata_getiq();

	FPGA_busy = false;
}

void FPGA_fpgadata_sendparam(void)
{
	uint32_t TRX_freq_phrase = getPhraseFromFrequency(CurrentVFO()->Freq);
	if (!TRX_on_TX())
	{
		switch (TRX_getMode())
		{
		case TRX_MODE_CW_L:
			TRX_freq_phrase = getPhraseFromFrequency(CurrentVFO()->Freq + TRX.CW_GENERATOR_SHIFT_HZ);
			break;
		case TRX_MODE_CW_U:
			TRX_freq_phrase = getPhraseFromFrequency(CurrentVFO()->Freq - TRX.CW_GENERATOR_SHIFT_HZ);
			break;
		default:
			break;
		}
	}
	//STAGE 2
	//out PTT+PREAMP
	FPGA_fpgadata_out_tmp8 = 0;
	bitWrite(FPGA_fpgadata_out_tmp8, 3, (TRX_on_TX() && TRX_getMode()!=TRX_MODE_LOOPBACK));
	if (!TRX_on_TX()) bitWrite(FPGA_fpgadata_out_tmp8, 2, TRX.Preamp);
	FPGA_writePacket(FPGA_fpgadata_out_tmp8);
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 3
	//out FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 20)) >> 20));
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 4
	//out FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 16)) >> 16));
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 5
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 12)) >> 12));
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 6
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 8)) >> 8));
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 7
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 4)) >> 4));
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 8
	//OUT FREQ
	FPGA_writePacket(TRX_freq_phrase & 0XF);
	//clock
	FPGA_clockRise();
	FPGA_clockFall();
}

void FPGA_fpgadata_getparam(void)
{
	//STAGE 2
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	TRX_ADC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 0);
	TRX_DAC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 1);
	//clock
	FPGA_clockFall();
}

void FPGA_fpgadata_getiq(void)
{
	FPGA_samples++;
	FPGA_fpgadata_in_tmp16 = 0;
	//STAGE 2
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XF)  << 12;
	//clock
	FPGA_clockFall();
	
	//STAGE 3
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 8;
	//clock
	FPGA_clockFall();

	//STAGE 4
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 4;
	//clock
	FPGA_clockFall();

	//STAGE 5
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF);
	FPGA_fpgadata_in_inttmp16 = FPGA_fpgadata_in_tmp16;

	if (FFTInputBufferInProgress) // A buffer in progress
	{
		if(TRX_IQ_swap)
		{
			FFTInput_A[FFT_buff_index] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] = FFTInput_A[FFT_buff_index];
		}
		else
		{
			FFTInput_A[FFT_buff_index + 1] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] = FFTInput_A[FFT_buff_index + 1];
		}
	}
	else // B buffer in progress
	{
		if(TRX_IQ_swap)
		{
			FFTInput_B[FFT_buff_index] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] = FFTInput_B[FFT_buff_index];
		}
		else
		{
			FFTInput_B[FFT_buff_index + 1] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] = FFTInput_B[FFT_buff_index + 1];
		}
	}
	//clock
	FPGA_clockFall();

	//STAGE 6
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XF) << 12;
	//clock
	FPGA_clockFall();

	//STAGE 7
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 8;
	//clock
	FPGA_clockFall();

	//STAGE 8
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 4;
	//clock
	FPGA_clockFall();

	//STAGE 9
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF);
	FPGA_fpgadata_in_inttmp16 = FPGA_fpgadata_in_tmp16;
	if (FFTInputBufferInProgress) // A buffer in progress
	{
		if(TRX_IQ_swap)
		{
			FFTInput_A[FFT_buff_index+1] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] = FFTInput_A[FFT_buff_index+1];
		}
		else
		{
			FFTInput_A[FFT_buff_index] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] = FFTInput_A[FFT_buff_index];
		}
	}
	else // B buffer in progress
	{
		if(TRX_IQ_swap)
		{
			FFTInput_B[FFT_buff_index+1] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] = FFTInput_B[FFT_buff_index+1];
		}
		else
		{
			FFTInput_B[FFT_buff_index] = (int16_t)FPGA_fpgadata_in_inttmp16;
			FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] = FFTInput_B[FFT_buff_index];
		}
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
	FPGA_clockFall();
}

void FPGA_fpgadata_sendiq(void)
{
	FPGA_samples++;
	//STAGE 2 out Q
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_SendBuffer_Q[FPGA_Audio_Buffer_Index];
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 12);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 3
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 8);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 4
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 4);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 5
	FPGA_writePacket(FPGA_fpgadata_out_tmp16);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 6 out I
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_SendBuffer_I[FPGA_Audio_Buffer_Index];
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 12);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 7
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 8);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 8
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 4);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 9
	FPGA_writePacket(FPGA_fpgadata_out_tmp16);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	FPGA_Audio_Buffer_Index++;
	if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE)
	{
		if (Processor_NeedTXBuffer) 
		{
			FPGA_Buffer_underrun = true;
			FPGA_Audio_Buffer_Index--;
		}
		else
		{
			FPGA_Audio_Buffer_Index = 0;
			FPGA_Audio_Buffer_State = true;
			Processor_NeedTXBuffer = true;
		}
	}
	else if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE / 2)
	{
		if (Processor_NeedTXBuffer)
		{
			FPGA_Buffer_underrun = true;
			FPGA_Audio_Buffer_Index--;
		}
		else
		{
			FPGA_Audio_Buffer_State = false;
			Processor_NeedTXBuffer = true;
		}
	}
}

inline void FPGA_clockRise(void)
{
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
}

inline void FPGA_clockFall(void)
{
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_RESET);
}

inline uint8_t FPGA_readPacket(void)
{
	return (((FPGA_IN_D0_GPIO_Port->IDR & FPGA_IN_D0_Pin) == FPGA_IN_D0_Pin) << 0) | (((FPGA_IN_D1_GPIO_Port->IDR & FPGA_IN_D1_Pin) == FPGA_IN_D1_Pin) << 1) | (((FPGA_IN_D2_GPIO_Port->IDR & FPGA_IN_D2_Pin) == FPGA_IN_D2_Pin) << 2) | (((FPGA_IN_D3_GPIO_Port->IDR & FPGA_IN_D3_Pin) == FPGA_IN_D3_Pin) << 3);
}

inline void FPGA_writePacket(uint8_t packet)
{
	FPGA_OUT_D0_GPIO_Port->BSRR = (bitRead(packet, 0) << 4 & FPGA_OUT_D0_Pin) | (bitRead(packet, 1) << 5 & FPGA_OUT_D1_Pin) | (bitRead(packet, 2) << 6 & FPGA_OUT_D2_Pin) | (bitRead(packet, 3) << 7 & FPGA_OUT_D3_Pin) | ((uint32_t)FPGA_OUT_D3_Pin << 16U) | ((uint32_t)FPGA_OUT_D2_Pin << 16U) | ((uint32_t)FPGA_OUT_D1_Pin << 16U) | ((uint32_t)FPGA_OUT_D0_Pin << 16U);
}
