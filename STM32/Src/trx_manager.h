#ifndef TRX_MANAGER_H
#define TRX_MANAGER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define FPGA_BUS_BITS 16 //разрядность данных из FPGA

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
extern void TRX_RF_UNIT_UpdateState(bool clean);
void TRX_setFrequency(int32_t _freq);
int32_t TRX_getFrequency(void);
void TRX_setMode(uint8_t _mode);
uint8_t TRX_getMode(void);
void TRX_ptt_change(void);
void TRX_key_change(void);
void TRX_Start_RX(void);
void TRX_Start_TX(void);
void TRX_Start_Loopback(void);
void TRX_Restart_Mode(void);
bool TRX_on_TX(void);
void TRX_DoAutoGain(void);

volatile extern bool TRX_ptt_hard;
volatile extern bool TRX_ptt_cat;
volatile extern bool TRX_old_ptt_cat;
volatile extern bool TRX_key_serial;
volatile extern bool TRX_old_key_serial;
volatile extern bool TRX_key_hard;
volatile extern uint16_t TRX_Key_Timeout_est;
volatile extern bool TRX_IQ_swap;
volatile extern bool TRX_squelched;
volatile extern bool TRX_tune;
volatile extern bool TRX_inited;
volatile extern int16_t TRX_RX_dBm;
volatile extern bool TRX_ADC_OTR;
volatile extern bool TRX_DAC_OTR;
volatile extern int16_t TRX_ADC_MINAMPLITUDE;
volatile extern int16_t TRX_ADC_MAXAMPLITUDE;
volatile extern uint8_t TRX_Time_InActive;

extern char *MODE_DESCR[];

#endif
