#ifndef TRX_MANAGER_H
#define TRX_MANAGER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define TRX_MODE_LSB 0
#define TRX_MODE_USB 1
#define TRX_MODE_IQ 2

void TRX_Init(void);
void TRX_setFrequency(int32_t _freq);
void TRX_setFrequencyPhrase(int32_t _phrase);
int32_t TRX_getFrequency(void);
int32_t TRX_getFrequencyPhrase(void);
void TRX_setMode(uint8_t _mode);
uint8_t TRX_getMode(void);
void TRX_ptt_change(void);

extern uint32_t TRX_freq;
extern uint32_t TRX_freq_phrase; //freq in hz/oscil in hz*2^bits = (freq/50000000)*4194304;
extern bool TRX_preamp;
extern bool TRX_hilbert;
extern bool TRX_agc;
extern bool TRX_loopback;
extern uint8_t TRX_gain_level;
extern uint8_t TRX_agc_speed;
extern bool TRX_ptt;
extern bool TRX_tune;
extern int32_t TRX_s_meter;
extern bool TRX_agc_wdsp_action;

#endif
