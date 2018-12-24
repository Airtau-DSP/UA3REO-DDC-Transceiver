#ifndef PROFILER_h
#define PROFILER_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define PROFILES_COUNT 7

typedef struct {
	uint32_t startTime;
	uint32_t endTime;
	uint32_t diff;
	bool started;
	uint32_t samples;
} PROFILE_INFO;

extern void InitProfiler(void);
extern void StartProfiler(uint8_t pid);
extern void EndProfiler(uint8_t pid);
extern void PrintProfilerResult(void);

#endif
