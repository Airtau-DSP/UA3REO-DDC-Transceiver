#ifndef Encoder_h
#define Encoder_h

#include "stm32f4xx_hal.h"

extern void ENCODER_Init(void);
extern void ENCODER_checkRotate(void);

extern RTC_HandleTypeDef hrtc;

#endif
