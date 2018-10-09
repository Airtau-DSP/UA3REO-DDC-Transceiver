#ifndef AUDIO_PROCESSOR_h
#define AUDIO_PROCESSOR_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"

#define APROCESSOR_BLOCK_SIZE 32

extern void processRxAudio(void);
extern void initAudioProcessor(void);
extern uint32_t AUDIOPROC_samples;
	
#endif
