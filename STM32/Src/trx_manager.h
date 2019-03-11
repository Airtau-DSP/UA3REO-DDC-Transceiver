#ifndef TRX_MANAGER_H
#define TRX_MANAGER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define MAX_FREQ_HZ 750000000 // From ADC Datasheet
#define CW_GENERATOR_SHIFT_HZ 500

#define TRX_MODE_LSB 0
#define TRX_MODE_USB 1
#define TRX_MODE_IQ 2
#define TRX_MODE_CW_L 3
#define TRX_MODE_CW_U 4
#define TRX_MODE_DIGI_L 5
#define TRX_MODE_DIGI_U 6
#define TRX_MODE_NO_TX 7
#define TRX_MODE_NFM 8
#define TRX_MODE_WFM 9
#define TRX_MODE_AM 10
#define TRX_MODE_LOOPBACK 11

void TRX_Init(void);
void TRX_setFrequency(int32_t _freq);
int32_t TRX_getFrequency(void);
void TRX_setMode(uint8_t _mode);
uint8_t TRX_getMode(void);
void TRX_ptt_change(void);
void TRX_Start_RX(void);
void TRX_Start_TX(void);
void TRX_Start_Loopback(void);
void TRX_Restart_Mode(void);

extern bool TRX_ptt;
extern bool TRX_squelched;
extern bool TRX_tune;
extern bool TRX_inited;
extern int32_t TRX_s_meter;
extern bool TRX_agc_wdsp_action;
extern bool TRX_ADC_OTR;
extern bool TRX_DAC_OTR;
extern char *MODE_DESCR[];

#endif
