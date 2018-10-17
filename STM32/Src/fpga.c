#include "stm32f4xx_hal.h"
#include "fpga.h"
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "audio_processor.h"

uint16_t FPGA_fpgadata_in_tmp16=0;
int16_t FPGA_fpgadata_in_inttmp16=0;
uint8_t FPGA_fpgadata_in_tmp8=0;
uint8_t FPGA_fpgadata_out_tmp8=0;

bool FPGA_busy=false;
GPIO_InitTypeDef FPGA_GPIO_InitStruct;

uint32_t FPGA_samples=0;

float32_t FPGA_Audio_IN_Buffer_Q_A[FPGA_AUDIO_BUFFER_SIZE]={0};
float32_t FPGA_Audio_IN_Buffer_I_A[FPGA_AUDIO_BUFFER_SIZE]={0};
float32_t FPGA_Audio_IN_Buffer_Q_B[FPGA_AUDIO_BUFFER_SIZE]={0};
float32_t FPGA_Audio_IN_Buffer_I_B[FPGA_AUDIO_BUFFER_SIZE]={0};
int16_t FPGA_Audio_IN_index=0;
uint8_t FPGA_Audio_IN_ActiveBuffer=0;
bool FPGA_Audio_IN_Buffer_Full_A=false;
bool FPGA_Audio_IN_Buffer_Full_B=false;

void FPGA_Init(void)
{
  //шина данных STM32-FPGA
	logToUART1_str("FPGA Bus Inited\r\n");
}

void FPGA_fpgadata_clock(void)
{
	if(FPGA_Audio_IN_ActiveBuffer==0 && FPGA_Audio_IN_Buffer_Full_A) return;
	if(FPGA_Audio_IN_ActiveBuffer==1 && FPGA_Audio_IN_Buffer_Full_B) return;
	FPGA_busy = true;
	//обмен данными
	
	//STAGE 1
	//out HILBERT+PTT+PREAMP
	FPGA_fpgadata_out_tmp8=0;
	bitWrite(FPGA_fpgadata_out_tmp8, 3, TRX_ptt);
	bitWrite(FPGA_fpgadata_out_tmp8, 1, TRX_hilbert);
	if (!TRX_ptt && !TRX_tune) bitWrite(FPGA_fpgadata_out_tmp8, 2, TRX_preamp);
	//logToUART1_num(FPGA_fpgadata_out_tmp8);
  FPGA_writePacket(FPGA_fpgadata_out_tmp8);
	//clock
	GPIOC->BSRR = FPGA_SYNC_Pin;
	HAL_GPIO_WritePin(FPGA_CLK_GPIO_Port,FPGA_CLK_Pin,GPIO_PIN_SET);
	FPGA_samples++;
	//in
	//clock
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);

	//STAGE 2
	//out FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 20)) >> 20));
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XF) << 12;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	//STAGE 3
	//out FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 16)) >> 16));
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 8;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
	
	//STAGE 4
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 12)) >> 12));
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 4;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
	
	//STAGE 5
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 8)) >> 8));
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in Q
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF);
	FPGA_fpgadata_in_inttmp16=FPGA_fpgadata_in_tmp16-32767; //32767
	//FPGA_fpgadata_in_inttmp16=FPGA_Audio_IN_index; //test signal
	FFTInput[FFT_buff_index+1]=((float32_t)FPGA_fpgadata_in_inttmp16); //32767 - half
	if(FPGA_Audio_IN_ActiveBuffer==0) FPGA_Audio_IN_Buffer_Q_A[FPGA_Audio_IN_index]=FFTInput[FFT_buff_index+1];
	if(FPGA_Audio_IN_ActiveBuffer==1) FPGA_Audio_IN_Buffer_Q_B[FPGA_Audio_IN_index]=FFTInput[FFT_buff_index+1];
	//FFT_buff_index++;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
	
	//STAGE 6
	//OUT FREQ
	FPGA_writePacket(((TRX_freq_phrase & (0XF << 4)) >> 4));
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 = (FPGA_readPacket() & 0XF) << 12;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
	
	//STAGE 7
	//OUT FREQ
	FPGA_writePacket(TRX_freq_phrase & 0XF);
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 8;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
	
	//STAGE 8
	//OUT
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF) << 4;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;
	
	//STAGE 9
	//out
	//clock
	GPIOC->BSRR = FPGA_CLK_Pin;
	//in I
	FPGA_fpgadata_in_tmp16 |= (FPGA_readPacket() & 0XF);
	FPGA_fpgadata_in_inttmp16=FPGA_fpgadata_in_tmp16-32767;
	FFTInput[FFT_buff_index]=((float32_t)FPGA_fpgadata_in_inttmp16); //32767 - half
	if(FPGA_Audio_IN_ActiveBuffer==0) FPGA_Audio_IN_Buffer_I_A[FPGA_Audio_IN_index]=FFTInput[FFT_buff_index];
	if(FPGA_Audio_IN_ActiveBuffer==1) FPGA_Audio_IN_Buffer_I_B[FPGA_Audio_IN_index]=FFTInput[FFT_buff_index];
	FPGA_Audio_IN_index++;
	FFT_buff_index+=2;
	if(FPGA_Audio_IN_ActiveBuffer==0 && FPGA_Audio_IN_index==FPGA_AUDIO_BUFFER_SIZE) { FPGA_Audio_IN_Buffer_Full_A=true; FPGA_Audio_IN_index=0; }
	if(FPGA_Audio_IN_ActiveBuffer==1 && FPGA_Audio_IN_index==FPGA_AUDIO_BUFFER_SIZE) { FPGA_Audio_IN_Buffer_Full_B=true; FPGA_Audio_IN_index=0; }
	if(FFT_buff_index==FFT_SIZE*2) FFT_buff_index=0;
	//clock
	GPIOC->BSRR = (uint32_t)FPGA_CLK_Pin << 16U;

	FPGA_busy=false;
}

inline uint8_t FPGA_readPacket(void)
{
  return (((FPGA_IN_D3_GPIO_Port->IDR & FPGA_IN_D3_Pin) == FPGA_IN_D3_Pin) << 3) | (((FPGA_IN_D2_GPIO_Port->IDR & FPGA_IN_D2_Pin) == FPGA_IN_D2_Pin) << 2) | (((FPGA_IN_D1_GPIO_Port->IDR & FPGA_IN_D1_Pin) == FPGA_IN_D1_Pin) << 1) | (((FPGA_IN_D0_GPIO_Port->IDR & FPGA_IN_D0_Pin) == FPGA_IN_D0_Pin));
}

inline void FPGA_writePacket(uint8_t packet)
{
	FPGA_OUT_D0_GPIO_Port->BSRR = (bitRead(packet, 0)<<9 & FPGA_OUT_D0_Pin) | (bitRead(packet, 1)<<8 & FPGA_OUT_D1_Pin) | (bitRead(packet, 2)<<7 & FPGA_OUT_D2_Pin)  | (bitRead(packet, 3)<<6 & FPGA_OUT_D3_Pin) | ((uint32_t)FPGA_OUT_D3_Pin << 16U) | ((uint32_t)FPGA_OUT_D2_Pin << 16U) | ((uint32_t)FPGA_OUT_D1_Pin << 16U) | ((uint32_t)FPGA_OUT_D0_Pin << 16U);
}
