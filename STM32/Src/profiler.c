#include "profiler.h"
#include "stm32f4xx_hal.h"
#include "string.h"
#include <stdbool.h>
#include "functions.h"

static PROFILE_INFO profiles[PROFILES_COUNT] = { 0 };

void InitProfiler()
{
	for (uint8_t i = 0; i < PROFILES_COUNT; i++)
	{
		profiles[i].startTime = 0;
		profiles[i].endTime = 0;
		profiles[i].diff = 0;
		profiles[i].samples = 0;
		profiles[i].started = false;
	}
}

void StartProfiler(uint8_t pid)
{
	if (pid >= PROFILES_COUNT) return;
	if (profiles[pid].started) return;
	profiles[pid].started = true;
	profiles[pid].startTime = HAL_GetTick();
}

void StartProfilerUs()
{
	if (profiles[PROFILES_COUNT-1].started) return;
	if(bitRead(DWT->CTRL, DWT_CTRL_CYCCNTENA_Pos)) return;
	profiles[PROFILES_COUNT-1].started = true;
	profiles[PROFILES_COUNT-1].startTime = 0;
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void EndProfiler(uint8_t pid, bool summarize)
{
	if (pid >= PROFILES_COUNT) return;
	if (!profiles[pid].started) return;
	profiles[pid].endTime = HAL_GetTick();
	if(summarize)
		profiles[pid].diff += profiles[pid].endTime - profiles[pid].startTime;
	else
		profiles[pid].diff = profiles[pid].endTime - profiles[pid].startTime;
	profiles[pid].samples++;
	profiles[pid].started = false;
}

void EndProfilerUs(bool summarize)
{
	if (!profiles[PROFILES_COUNT-1].started) return;
	profiles[PROFILES_COUNT-1].endTime = DWT->CYCCNT / (SystemCoreClock / 1000000);
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
	DWT->CYCCNT = 0;
	if(summarize)
		profiles[PROFILES_COUNT-1].diff += profiles[PROFILES_COUNT-1].endTime;
	else
		profiles[PROFILES_COUNT-1].diff = profiles[PROFILES_COUNT-1].endTime;
	profiles[PROFILES_COUNT-1].samples++;
	profiles[PROFILES_COUNT-1].started = false;
}

void PrintProfilerResult()
{
	bool printed = false;
	for (uint8_t i = 0; i < PROFILES_COUNT; i++)
		if (profiles[i].samples > 0)
		{
			sendToDebug_str("Profile #");
			sendToDebug_uint8(i, true);
			sendToDebug_str(" Samples: ");
			sendToDebug_uint32(profiles[i].samples, true);
			if(i==PROFILES_COUNT-1)
				sendToDebug_str(" Time, us: ");
			else
				sendToDebug_str(" Time, ms: ");
			sendToDebug_uint32(profiles[i].diff, false);
			profiles[i].diff = 0;
			profiles[i].samples = 0;
			printed = true;
		}
	if (printed)
		sendToDebug_str("\r\n");
}
