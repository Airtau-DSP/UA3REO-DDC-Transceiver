#include "stm32f4xx_hal.h"
#include "main.h"
#include "fpga.h"
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "audio_processor.h"
#include "settings.h"
#include "wm8731.h"

volatile uint32_t FPGA_samples = 0;
volatile bool FPGA_busy = false;
volatile float32_t FPGA_MAX_I_Value = 0;
volatile float32_t FPGA_MIN_I_Value = 0;
volatile float32_t FPGA_DC_Offset = 0;
volatile bool FPGA_NeedSendParams = false;
volatile bool FPGA_NeedGetParams = false;
volatile bool FPGA_Buffer_underrun = false;

uint8_t FPGA_fpgadata_in_tmp8 = 0;
uint8_t FPGA_fpgadata_out_tmp8 = 0;
GPIO_InitTypeDef FPGA_GPIO_InitStruct;
bool FPGA_bus_direction = false; //false - OUT; true - in
uint16_t FPGA_Audio_Buffer_Index = 0;
bool FPGA_Audio_Buffer_State = true; //true - compleate ; false - half
float32_t FPGA_Audio_Buffer_SPEC_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_SPEC_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_VOICE_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_VOICE_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_SendBuffer_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_SendBuffer_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };

void FPGA_Init(void)
{
	FPGA_GPIO_InitStruct.Pin = FPGA_BUS_D0_Pin|FPGA_BUS_D1_Pin|FPGA_BUS_D2_Pin|FPGA_BUS_D3_Pin|FPGA_BUS_D4_Pin|FPGA_BUS_D5_Pin|FPGA_BUS_D6_Pin|FPGA_BUS_D7_Pin;
	FPGA_GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	FPGA_GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	FPGA_GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOA, &FPGA_GPIO_InitStruct);
	
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
	FPGA_writePacket(((TRX_freq_phrase & (0XFF << 16)) >> 16));
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 4
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XFF << 8)) >> 8));
	//clock
	FPGA_clockRise();
	FPGA_clockFall();

	//STAGE 5
	//OUT FREQ
	FPGA_writePacket(TRX_freq_phrase & 0XFF);
	//clock
	FPGA_clockRise();
	FPGA_clockFall();
}

void FPGA_fpgadata_getparam(void)
{
	uint16_t adc_max=0;
	//STAGE 2
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	adc_max = (FPGA_fpgadata_in_tmp8 & 0X3C) << 6;
	TRX_ADC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 0);
	TRX_DAC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 1);
	//clock
	FPGA_clockFall();
	//STAGE 3
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	adc_max |= (FPGA_fpgadata_in_tmp8 & 0XFF);
	TRX_ADC_MAXAMPLITUDE = adc_max;
	//clock
	FPGA_clockFall();
}

void FPGA_fpgadata_getiq(void)
{
	int16_t FPGA_fpgadata_in_tmp16 = 0;
	float32_t FPGA_fpgadata_iq_corrected = 0;
	FPGA_samples++;
	
	//STAGE 2
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XFF) << 8;
	//clock
	FPGA_clockFall();

	//STAGE 3
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XFF);
	FPGA_fpgadata_iq_corrected=FPGA_fpgadata_in_tmp16+FPGA_DC_Offset;
	
	if(TRX_IQ_swap)
	{
		if(NeedFFTInputBuffer) FFTInput_I[FFT_buff_index] = FPGA_fpgadata_iq_corrected;
		FPGA_Audio_Buffer_SPEC_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_iq_corrected;
	}
	else
	{
		if(NeedFFTInputBuffer) FFTInput_Q[FFT_buff_index] = FPGA_fpgadata_iq_corrected;
		FPGA_Audio_Buffer_SPEC_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_iq_corrected;
	}
	
	//clock
	FPGA_clockFall();

	//STAGE 4
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XFF) << 8;
	//clock
	FPGA_clockFall();

	//STAGE 5
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XFF);
	FPGA_fpgadata_iq_corrected=FPGA_fpgadata_in_tmp16+FPGA_DC_Offset;
	
	if(TRX_IQ_swap)
	{
		if(NeedFFTInputBuffer) FFTInput_Q[FFT_buff_index] = FPGA_fpgadata_iq_corrected;
		FPGA_Audio_Buffer_SPEC_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_iq_corrected;
	}
	else
	{
		if(NeedFFTInputBuffer) FFTInput_I[FFT_buff_index] = FPGA_fpgadata_iq_corrected;
		FPGA_Audio_Buffer_SPEC_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_iq_corrected;
	}
	
	//clock
	FPGA_clockFall();
	
	//STAGE 6
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XFF) << 8;
	//clock
	FPGA_clockFall();

	//STAGE 7
	//clock
	FPGA_clockRise();
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XFF);
	
	if(TRX_IQ_swap)
		FPGA_Audio_Buffer_VOICE_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	else
		FPGA_Audio_Buffer_VOICE_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;

	//clock
	FPGA_clockFall();

	//STAGE 8
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XFF) << 8;
	//clock
	FPGA_clockFall();

	//STAGE 9
	//clock
	FPGA_clockRise();
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XFF);
	
	if(TRX_IQ_swap)
		FPGA_Audio_Buffer_VOICE_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	else
		FPGA_Audio_Buffer_VOICE_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	
	FPGA_Audio_Buffer_Index++;
	if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE) FPGA_Audio_Buffer_Index = 0;
	
	if(NeedFFTInputBuffer)
	{
		FFT_buff_index++;
		if (FFT_buff_index == FFT_SIZE)
		{
			FFT_buff_index = 0;
			NeedFFTInputBuffer = false;
		}
	}
	//clock
	FPGA_clockFall();
}

void FPGA_fpgadata_sendiq(void)
{
	int16_t FPGA_fpgadata_out_tmp16 = 0;
	
	FPGA_samples++;
	//STAGE 2 out Q
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_SendBuffer_Q[FPGA_Audio_Buffer_Index];
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 8);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 3
	FPGA_writePacket(FPGA_fpgadata_out_tmp16);
	//clock
	FPGA_clockRise();
	//clock
	FPGA_clockFall();

	//STAGE 4 out I
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_SendBuffer_I[FPGA_Audio_Buffer_Index];
	FPGA_writePacket(FPGA_fpgadata_out_tmp16 >> 8);
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

void FPGA_setDirectionFast(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
	uint32_t position;
	uint32_t ioposition = 0x00U;
	uint32_t iocurrent = 0x00U;
	uint32_t temp = 0x00U;
	
	for(position = 0U; position < 16U; position++)
	{
		ioposition = 0x01U << position;
		iocurrent = (uint32_t)(GPIO_Init->Pin) & ioposition;

		if(iocurrent == ioposition)
		{
			// Configure IO Direction mode (Input, Output, Alternate or Analog)
			temp = GPIOx->MODER;
			temp &= ~(GPIO_MODER_MODER0 << (position * 2U));
			temp |= ((GPIO_Init->Mode & 0x00000003U) << (position * 2U));
			GPIOx->MODER = temp;
		
			if(GPIO_Init->Mode == GPIO_MODE_OUTPUT_PP)
			{
				// Configure the IO Output Type
				temp = GPIOx->OTYPER;
				temp &= ~(GPIO_OTYPER_OT_0 << position) ;
				temp |= (((GPIO_Init->Mode & 0x00000010U) >> 4U) << position);
				GPIOx->OTYPER = temp;
			}
		}
	}
}

void FPGA_setBusInput(void)
{
	FPGA_GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	FPGA_setDirectionFast(GPIOA, &FPGA_GPIO_InitStruct);
	FPGA_bus_direction=true;
}

void FPGA_setBusOutput(void)
{
	FPGA_GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	FPGA_setDirectionFast(GPIOA, &FPGA_GPIO_InitStruct);
	FPGA_bus_direction=false;
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
	if(!FPGA_bus_direction)
		FPGA_setBusInput();
	return (((FPGA_BUS_D0_GPIO_Port->IDR & FPGA_BUS_D0_Pin) == FPGA_BUS_D0_Pin) << 0) 
				| (((FPGA_BUS_D1_GPIO_Port->IDR & FPGA_BUS_D1_Pin) == FPGA_BUS_D1_Pin) << 1) 
				| (((FPGA_BUS_D2_GPIO_Port->IDR & FPGA_BUS_D2_Pin) == FPGA_BUS_D2_Pin) << 2) 
				| (((FPGA_BUS_D3_GPIO_Port->IDR & FPGA_BUS_D3_Pin) == FPGA_BUS_D3_Pin) << 3) 
				| (((FPGA_BUS_D4_GPIO_Port->IDR & FPGA_BUS_D4_Pin) == FPGA_BUS_D4_Pin) << 4) 
				| (((FPGA_BUS_D5_GPIO_Port->IDR & FPGA_BUS_D5_Pin) == FPGA_BUS_D5_Pin) << 5) 
				| (((FPGA_BUS_D6_GPIO_Port->IDR & FPGA_BUS_D6_Pin) == FPGA_BUS_D6_Pin) << 6) 
				| (((FPGA_BUS_D7_GPIO_Port->IDR & FPGA_BUS_D7_Pin) == FPGA_BUS_D7_Pin) << 7);
}

inline void FPGA_writePacket(uint8_t packet)
{
	if(FPGA_bus_direction)
		FPGA_setBusOutput();
	FPGA_BUS_D0_GPIO_Port->BSRR = 
				(bitRead(packet, 0) << 0 & FPGA_BUS_D0_Pin) 
				| (bitRead(packet, 1) << 1 & FPGA_BUS_D1_Pin) 
				| (bitRead(packet, 2) << 2 & FPGA_BUS_D2_Pin) 
				| (bitRead(packet, 3) << 3 & FPGA_BUS_D3_Pin) 
				| (bitRead(packet, 4) << 4 & FPGA_BUS_D4_Pin) 
				| (bitRead(packet, 5) << 5 & FPGA_BUS_D5_Pin) 
				| (bitRead(packet, 6) << 6 & FPGA_BUS_D6_Pin) 
				| (bitRead(packet, 7) << 7 & FPGA_BUS_D7_Pin) 
				| ((uint32_t)FPGA_BUS_D7_Pin << 16U) 
				| ((uint32_t)FPGA_BUS_D6_Pin << 16U) 
				| ((uint32_t)FPGA_BUS_D5_Pin << 16U) 
				| ((uint32_t)FPGA_BUS_D4_Pin << 16U)
				| ((uint32_t)FPGA_BUS_D3_Pin << 16U)
				| ((uint32_t)FPGA_BUS_D2_Pin << 16U)
				| ((uint32_t)FPGA_BUS_D1_Pin << 16U)
				| ((uint32_t)FPGA_BUS_D0_Pin << 16U);
}
