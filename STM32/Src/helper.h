#ifndef HELPER_h
#define HELPER_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define HELPER_ENABLED 0

extern UART_HandleTypeDef huart6;

extern void HELPER_updateSettings(void);

void HELPER_setBPF(uint8_t val);
void HELPER_setAMP(bool val);
void HELPER_setAMP_POWER(bool val);
void HELPER_setPREAMP(bool val);
void HELPER_setATT(bool val);
void HELPER_WriteCommand(char* data);
uint8_t HELPER_getBPFfromFreq(uint32_t freq);

#endif
