#ifndef Encoder_h
#define Encoder_h

#include "stm32f4xx_hal.h"

#define ENCODER_RATE 20 //Encoder slowing rate

void ENCODER_Init(void);
void ENCODER_checkRotate(void);
void ENCODER_Rotated(int direction);

extern int ENCODER_ALast;
extern int ENCODER_AVal;
extern RTC_HandleTypeDef hrtc;

#endif
