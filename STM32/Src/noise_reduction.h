#ifndef NOISE_REDUCTION_h
#define NOISE_REDUCTION_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"

#define NOISE_REDUCTION_BLOCK_SIZE 128
#define NOISE_REDUCTION_TAPS 16
#define NOISE_REDUCTION_REFERENCE_SIZE NOISE_REDUCTION_BLOCK_SIZE*2
#define NOISE_REDUCTION_STEP 0.000001f

extern void InitNoiseReduction(void);
extern void processNoiseReduction(float* bufferIn, float* bufferOut);

#endif
