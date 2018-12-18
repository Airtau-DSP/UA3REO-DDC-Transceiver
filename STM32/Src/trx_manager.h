#ifndef TRX_MANAGER_H
#define TRX_MANAGER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define TRX_MODE_LSB 0
#define TRX_MODE_USB 1
#define TRX_MODE_IQ 2
#define TRX_MODE_CW 3
#define TRX_MODE_DIGI_L 4
#define TRX_MODE_DIGI_U 5
#define TRX_MODE_NO_TX 6
#define TRX_MODE_FM 7
#define TRX_MODE_AM 8
#define TRX_MODE_LOOPBACK 9

void TRX_Init(void);
void TRX_setFrequency(uint32_t _freq);
int32_t TRX_getFrequency(void);
void TRX_setMode(uint8_t _mode);
uint8_t TRX_getMode(void);
void TRX_ptt_change(void);

extern bool TRX_ptt;
extern bool TRX_tune;
extern bool TRX_inited;
extern int32_t TRX_s_meter;
extern bool TRX_agc_wdsp_action;
extern bool TRX_ADC_OTR;
extern bool TRX_DAC_OTR;
extern char *MODE_DESCR[10];

#endif
