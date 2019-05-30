#include "agc.h"
#include "stm32f4xx_hal.h"
#include "math.h"
#include "arm_math.h"
#include "functions.h"
#include "settings.h"
#include "profiler.h"

static float32_t AGC_need_gain = 0.0f;
static float32_t RX_AGC_STEPSIZE_UP = 1.0f;
static float32_t RX_AGC_STEPSIZE_DOWN = 1.0f;
static float32_t AGC_need_gain_old = 1.0f;

void InitAGC(void)
{
	//выше скорость в настройках - выше скорость отработки AGC
	RX_AGC_STEPSIZE_UP = 500.0f / (float32_t)TRX.Agc_speed;
	RX_AGC_STEPSIZE_DOWN = RX_AGC_STEPSIZE_UP / 10.0f;
}

void DoAGC(float32_t *agcBuffer, int16_t blockSize)
{
	float32_t AGC_RX_MAX_amplitude = 0;
	uint32_t AGC_RX_MAX_amplitude_index = 0;
	//ищем максимум в амплитуде
	arm_max_f32(agcBuffer, blockSize, &AGC_RX_MAX_amplitude, &AGC_RX_MAX_amplitude_index);
	if (AGC_RX_MAX_amplitude == 0.0f) AGC_RX_MAX_amplitude = 0.001f;
	//расчитываем целевое значение усиления
	float32_t AGC_need_gain_target = AGC_OPTIMAL_THRESHOLD / AGC_RX_MAX_amplitude;
	//двигаем усиление на шаг
	if (AGC_need_gain_target > AGC_need_gain)
	{
		float32_t step = (AGC_need_gain_target - AGC_need_gain) / RX_AGC_STEPSIZE_UP;
		if (step > 1.0f) step = 1.0f;
		AGC_need_gain += step;
	}
	else
		AGC_need_gain -= (AGC_need_gain - AGC_need_gain_target) / RX_AGC_STEPSIZE_DOWN;
	if (AGC_need_gain < 0.0f) AGC_need_gain = 0.0f;
	//перегрузка (клиппинг), резко снижаем усиление
	if ((AGC_need_gain*AGC_RX_MAX_amplitude) > AGC_CLIP_THRESHOLD)
	{
		AGC_need_gain = AGC_need_gain_target;
		//sendToDebug_str("RX AGC Clip");
	}
	//AGC выключен, ничего не усиливаем (требуется для плавного выключения)
	if (!TRX.AGC || TRX_getMode() == TRX_MODE_DIGI_L || TRX_getMode() == TRX_MODE_DIGI_U)
		AGC_need_gain = 1.0f;
	//применяем усиление
	if (AGC_need_gain_old != AGC_need_gain) //усиление изменилось
	{
		float32_t gainApplyStep = 0;
		if (AGC_need_gain_old > AGC_need_gain)
			gainApplyStep = -(AGC_need_gain_old - AGC_need_gain) / blockSize;
		if (AGC_need_gain_old < AGC_need_gain)
			gainApplyStep = (AGC_need_gain - AGC_need_gain_old) / blockSize;
		for (uint16_t i = 0; i < blockSize; i++)
		{
			AGC_need_gain_old += gainApplyStep;
			agcBuffer[i] = agcBuffer[i] * AGC_need_gain_old;
		}
	}
	else //усиление не менялось, применяем усиление по всем выборкам
	{
		arm_scale_f32(agcBuffer, AGC_need_gain, agcBuffer, blockSize);
	}
}
