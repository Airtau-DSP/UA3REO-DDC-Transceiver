#ifndef FPGA_h
#define FPGA_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "fft.h"
#include "audio_processor.h"

#define FPGA_AUDIO_BUFFER_SIZE 1024

void FPGA_Init(void);
void FPGA_fpgadata_clock(void);
uint8_t FPGA_readPacket(void);
void FPGA_writePacket(uint8_t packet);
	
extern bool FPGA_busy;
extern uint32_t FPGA_samples;

extern float32_t FPGA_Audio_IN_Buffer_Q_A[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_IN_Buffer_I_A[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_IN_Buffer_Q_B[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_IN_Buffer_I_B[FPGA_AUDIO_BUFFER_SIZE];
extern uint8_t FPGA_Audio_IN_ActiveBuffer;
extern bool FPGA_Audio_IN_Buffer_Full_A;
extern bool FPGA_Audio_IN_Buffer_Full_B;

#endif
