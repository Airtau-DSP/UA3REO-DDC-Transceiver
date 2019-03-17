#ifndef TRX_MANAGER_H
#define TRX_MANAGER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define MAX_FREQ_HZ 750000000 // From ADC Datasheet

#define ADC_BITS 12 //разрядность АЦП
#define FPGA_BUS_BITS 16 //разрядность данных из FPGA
#define ADC_MAX_VALUE_UNSIGNED 4095 //максимальное возможное значение из АЦП
#define ADC_MAX_VALUE_SIGNED 2047 //максимальное возможное значение из АЦП с учётом знака
#define ADC_VREF 1.0f //опорное напряжение АЦП, при подаче на вход которого АЦП отдаёт максимальное значение, вольт
#define ADC_RESISTANCE 200 //сопротивление входа АЦП, ом
#define ADC_RF_TRANS_RATIO 4 //коэффициент трансформации трансформатора :) на входе АЦП
#define ADC_RF_INPUT_VALUE_CALIBRATION 2.0f; //коэффициент, на который умножаем данные с АЦП, чтобы получить реальное напряжение, устанавливается при калибровке трансивера

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
extern int16_t TRX_RX_dBm;
extern bool TRX_agc_wdsp_action;
extern bool TRX_ADC_OTR;
extern bool TRX_DAC_OTR;
extern char *MODE_DESCR[];
extern uint8_t TRX_Time_InActive;

#endif
