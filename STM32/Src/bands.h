#ifndef BANDS_H
#define BANDS_H

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdbool.h>
#include "trx_manager.h"

#define BANDS_COUNT 13

typedef struct {
	const uint32_t startFreq;
	const uint32_t endFreq;
	const uint8_t mode;
} REGION_MAP;

typedef struct {
	const char* name;
	const uint32_t startFreq;
	const uint32_t endFreq;
	const REGION_MAP* regions;
	const uint8_t regionsCount;
} BAND_MAP;

extern uint8_t getModeFromFreq(uint32_t freq);

#endif
