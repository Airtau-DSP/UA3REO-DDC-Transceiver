#include "stm32f4xx_hal.h"
#include "fpga.h"
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "audio_processor.h"
#include "settings.h"
#include "wm8731.h"

uint16_t FPGA_fpgadata_in_tmp16 = 0;
uint16_t FPGA_fpgadata_out_tmp16 = 0;
int16_t FPGA_fpgadata_in_inttmp16 = 0;
uint8_t FPGA_fpgadata_in_tmp8 = 0;
uint8_t FPGA_fpgadata_out_tmp8 = 0;

bool FPGA_busy = false;
GPIO_InitTypeDef FPGA_GPIO_InitStruct;

uint32_t FPGA_samples = 0;

float32_t FPGA_Audio_Buffer_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
uint16_t FPGA_Audio_Buffer_Index = 0;

bool FPGA_NeedSendParams = false;
bool FPGA_NeedGetParams = false;

void FPGA_Init(void)
{
	//шина данных STM32-FPGA
	//logToUART1_str("FPGA Bus Inited\r\n");
}

void FPGA_fpgadata_clock(void)
{
	FPGA_busy = true;
	//обмен данными

	//STAGE 1
	//out
	if (FPGA_NeedSendParams) FPGA_fpgadata_out_tmp8 = 1;
	else if (FPGA_NeedGetParams)  FPGA_fpgadata_out_tmp8 = 2;
	else if (TRX_ptt) FPGA_fpgadata_out_tmp8 = 3;
	else FPGA_fpgadata_out_tmp8 = 4;

	FPGA_writePacket(FPGA_fpgadata_out_tmp8);
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port, FPGA_CLK_Pin, GPIO_PIN_SET);
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

	if (FPGA_NeedSendParams) { FPGA_fpgadata_sendparam(); FPGA_NeedSendParams = false; }
	else if (FPGA_NeedGetParams) { FPGA_fpgadata_getparam(); FPGA_NeedGetParams = false; }
	else if (TRX_ptt) FPGA_fpgadata_sendiq();
	else FPGA_fpgadata_getiq();

	FPGA_busy = false;
}

void FPGA_fpgadata_sendparam(void)
{
	//STAGE 2
	//out PTT+PREAMP
	FPGA_fpgadata_out_tmp8 = 0;
	bitWrite(FPGA_fpgadata_out_tmp8, 3, TRX_ptt);
	if (!TRX_ptt && !TRX.Tune) bitWrite(FPGA_fpgadata_out_tmp8, 2, TRX.Preamp);
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
	FPGA_fpgadata_in_inttmp16 = FPGA_fpgadata_in_tmp16; //-32767
	//logToUART1_int16(FPGA_fpgadata_in_inttmp16);
	//FPGA_fpgadata_in_inttmp16=FPGA_Audio_Buffer_Index; //test signal
	FFTInput[FFT_buff_index + 1] = ((float32_t)FPGA_fpgadata_in_inttmp16);
	FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index] = FFTInput[FFT_buff_index + 1];
	//FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index]=0;
	//FFT_buff_index++;
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
	FPGA_fpgadata_in_inttmp16 = FPGA_fpgadata_in_tmp16; //-32767
	//FPGA_fpgadata_in_inttmp16=FPGA_Audio_Buffer_Index; //test signal
	FFTInput[FFT_buff_index] = ((float32_t)FPGA_fpgadata_in_inttmp16);
	FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index] = FFTInput[FFT_buff_index];
	//FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index]=0;
	FPGA_Audio_Buffer_Index++;
	FFT_buff_index += 2;
	if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE) FPGA_Audio_Buffer_Index = 0;
	if (FFT_buff_index == FFT_SIZE * 2) FFT_buff_index = 0;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
}

void FPGA_fpgadata_sendiq(void)
{
	FPGA_samples++;
	
	//STAGE 2 out Q
	FPGA_fpgadata_out_tmp16 = FPGA_Audio_Buffer_Q[FPGA_Audio_Buffer_Index];
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
	FPGA_fpgadata_out_tmp16 = FPGA_Audio_Buffer_I[FPGA_Audio_Buffer_Index];
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
		FPGA_Audio_Buffer_Index = 0;
		WM8731_DMA_state = true;
		Processor_NeedBuffer = true;
	}
	else if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE / 2)
	{
		WM8731_DMA_state = false;
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
