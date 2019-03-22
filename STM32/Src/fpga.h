#ifndef FPGA_h
#define FPGA_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "fft.h"
#include "audio_processor.h"

#define ADCDAC_CLOCK 50000000
#define MAX_TX_AMPLITUDE 32767.0f
#define HALF_TX_AMPLITUDE 16383.5f
#define TUNE_AMPLITUDE MAX_TX_AMPLITUDE

void FPGA_Init(void);
void FPGA_fpgadata_iqclock(void);
void FPGA_fpgadata_stuffclock(void);
uint8_t FPGA_readPacket(void);
void FPGA_writePacket(uint8_t packet);

extern bool FPGA_busy;
extern uint32_t FPGA_samples;
extern bool FPGA_Buffer_underrun;

extern float32_t FPGA_Audio_Buffer_Q[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_Buffer_I[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_SendBuffer_Q[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_SendBuffer_I[FPGA_AUDIO_BUFFER_SIZE];
extern uint16_t FPGA_Audio_Buffer_Index;
extern bool FPGA_Audio_Buffer_State;

extern bool FPGA_NeedSendParams;
extern bool FPGA_NeedGetParams;
void FPGA_fpgadata_sendparam(void);
void FPGA_fpgadata_getparam(void);
void FPGA_fpgadata_getiq(void);
void FPGA_fpgadata_sendiq(void);
void FPGA_testbus(void);
extern void FPGA_start_audio_clock(void);
extern void FPGA_stop_audio_clock(void);

#endif
