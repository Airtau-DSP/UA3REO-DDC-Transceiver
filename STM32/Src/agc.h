#ifndef AGC_H
#define AGC_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "arm_math.h"
#include "audio_processor.h"

extern void DoAGC(float32_t *agcbuffer, int16_t blockSize);
extern void InitAGC(void);

#endif
