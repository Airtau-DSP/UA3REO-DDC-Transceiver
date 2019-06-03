#ifndef CW_DECODER_h
#define CW_DECODER_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"

#define CWDECODER_TARGET_FREQ 350.0
#define CWDECODER_SAMPLES 192
#define CWDECODER_HIGH_AVERAGE 100
#define CWDECODER_LOW_AVERAGE (CWDECODER_HIGH_AVERAGE*10)
#define CWDECODER_NBTIME 6  /// ms noise blanker
#define CWDECODER_STRLEN 15

extern volatile uint16_t CW_Decoder_WPM;
extern char CW_Decoder_Text[CWDECODER_STRLEN];

extern void CWDecoder_Init(void);
extern void CWDecoder_Process(float32_t* bufferIn);

#endif
