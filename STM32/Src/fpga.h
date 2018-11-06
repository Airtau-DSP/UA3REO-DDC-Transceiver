#ifndef FPGA_h
#define FPGA_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "fft.h"
#include "audio_processor.h"

#define ADCDAC_CLOCK 50000000

void FPGA_Init(void);
void FPGA_fpgadata_clock(void);
uint8_t FPGA_readPacket(void);
void FPGA_writePacket(uint8_t packet);

extern bool FPGA_busy;
extern uint32_t FPGA_samples;

extern float32_t FPGA_Audio_Buffer_Q[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_Buffer_I[FPGA_AUDIO_BUFFER_SIZE];
extern uint16_t FPGA_Audio_Buffer_Index;
extern bool FPGA_Audio_Buffer_State;

extern bool FPGA_NeedSendParams;
extern bool FPGA_NeedGetParams;
void FPGA_fpgadata_sendparam(void);
void FPGA_fpgadata_getparam(void);
void FPGA_fpgadata_getiq(void);
void FPGA_fpgadata_sendiq(void);

#endif
