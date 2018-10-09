#ifndef NOTCH_FILTER_h
#define NOTCH_FILTER_h

#include "stm32f4xx_hal.h"
#include "arm_math.h"

extern void InitNotchFilter(void);
extern void NotchFilter(int16_t blockSize, float32_t *notchbuffer);

#endif
