#ifndef FPGA_h
#define FPGA_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "fft.h"
#include "audio_processor.h"
#include "settings.h"

#define HALF_TX_AMPLITUDE MAX_TX_AMPLITUDE/2.0f

void FPGA_Init(void);
void FPGA_fpgadata_iqclock(void);
void FPGA_fpgadata_stuffclock(void);
uint8_t FPGA_readPacket(void);
void FPGA_writePacket(uint8_t packet);
void FPGA_clockRise(void);
void FPGA_clockFall(void);

volatile extern bool FPGA_busy;
volatile extern uint32_t FPGA_samples;
volatile extern bool FPGA_Buffer_underrun;
volatile extern bool FPGA_NeedSendParams;
volatile extern bool FPGA_NeedGetParams;

extern float32_t FPGA_Audio_Buffer_SPEC_Q[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_Buffer_SPEC_I[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_Buffer_VOICE_Q[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_Buffer_VOICE_I[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_SendBuffer_Q[FPGA_AUDIO_BUFFER_SIZE];
extern float32_t FPGA_Audio_SendBuffer_I[FPGA_AUDIO_BUFFER_SIZE];
extern uint16_t FPGA_Audio_Buffer_Index;
extern bool FPGA_Audio_Buffer_State;

void FPGA_fpgadata_sendparam(void);
void FPGA_fpgadata_getparam(void);
void FPGA_fpgadata_getiq(void);
void FPGA_fpgadata_sendiq(void);
void FPGA_testbus(void);
extern void FPGA_start_audio_clock(void);
extern void FPGA_stop_audio_clock(void);

#endif
