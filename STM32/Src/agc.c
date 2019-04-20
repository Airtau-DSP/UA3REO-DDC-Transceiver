#include "agc.h"
#include "stm32f4xx_hal.h"
#include "math.h"
#include "arm_math.h"
#include "functions.h"
#include "settings.h"
#include "profiler.h"

float32_t AGC_need_gain = 0.0f;
float32_t RX_AGC_STEPSIZE_UP=1.0f;
float32_t RX_AGC_STEPSIZE_DOWN=1.0f;
float32_t AGC_RX_MAX_amplitude=0;
uint32_t AGC_RX_MAX_amplitude_index=0;
	
void InitAGC(void)
{
	//выше скорость в настройках - выше скорость отработки AGC
	RX_AGC_STEPSIZE_UP=5000.0f/(float32_t)TRX.Agc_speed;
	RX_AGC_STEPSIZE_DOWN=50.0f/(float32_t)TRX.Agc_speed;
}

void DoAGC(float32_t *agcBuffer, int16_t blockSize)
{
	//ищем максимум в амплитуде
	arm_max_f32(agcBuffer,blockSize,&AGC_RX_MAX_amplitude,&AGC_RX_MAX_amplitude_index);
	if(AGC_RX_MAX_amplitude==0.0f) AGC_RX_MAX_amplitude=0.001f;
	//расчитываем целевое значение усиления
	float32_t AGC_need_gain_target = AGC_OPTIMAL_THRESHOLD/AGC_RX_MAX_amplitude;
	//двигаем усиление на шаг
	if(AGC_need_gain_target>AGC_need_gain)
	{
		float32_t step=(AGC_need_gain_target-AGC_need_gain)/RX_AGC_STEPSIZE_UP;
		if(step>1.0f) step=1.0f;
		AGC_need_gain += step;
	}
	else
		AGC_need_gain -= (AGC_need_gain-AGC_need_gain_target)/RX_AGC_STEPSIZE_DOWN;
	if(AGC_need_gain<0.0f) AGC_need_gain=0.0f;
	//перегрузка (клиппинг), резко снижаем усиление
	if((AGC_need_gain*AGC_RX_MAX_amplitude)>AGC_CLIP_THRESHOLD)
		AGC_need_gain = AGC_need_gain_target;
	//применяем усиление
	arm_scale_f32(agcBuffer, AGC_need_gain, agcBuffer, blockSize);
}
