#ifndef AGC_H
#define AGC_H

//AGC MODULE FROM mcHF QRP Transceiver K Atanassov - M0NKA 2014

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "arm_math.h"

#define ADC_CLIP_WARN_THRESHOLD 30000

#define agc_wdsp_slope 70
#define agc_wdsp_hang_time 500
#define agc_wdsp_mode TRX.Agc_speed
#define agc_wdsp_tau_hang_decay 200
#define agc_wdsp_thresh 60
#define agc_wdsp_hang_thresh 45

typedef struct
{
	// AGC
	//#define MAX_SAMPLE_RATE     48000.0f
	//#define MAX_N_TAU           8.0f
	//#define MAX_TAU_ATTACK      0.01f
	//#define RB_SIZE       (int) (MAX_SAMPLE_RATE * MAX_N_TAU * MAX_TAU_ATTACK + 1)
	//#define AGC_WDSP_RB_SIZE (int)(MAX_SAMPLE_RATE * MAX_N_TAU * MAX_TAU_ATTACK + 1) //384
#define AGC_WDSP_RB_SIZE 400 //384

	int pmode;// = 1; // if 0, calculate magnitude by max(|I|, |Q|), if 1, calculate sqrtf(I*I+Q*Q)
	float32_t out_sample[2];
	float32_t abs_out_sample;
	float32_t tau_attack;
	float32_t tau_decay;
	int n_tau;
	float32_t max_gain;
	float32_t var_gain;
	float32_t max_input;
	float32_t out_targ;
	float32_t tau_fast_backaverage;
	float32_t tau_fast_decay;
	float32_t pop_ratio;
	//uint8_t hang_enable;
	float32_t tau_hang_backmult;
	float32_t hangtime;
	float32_t hang_thresh;
	float32_t tau_hang_decay;
	float32_t ring[2 * AGC_WDSP_RB_SIZE]; //192]; //96];
	float32_t abs_ring[AGC_WDSP_RB_SIZE];// 192 //96]; // abs_ring is half the size of ring
	//assign constants
	int ring_buffsize; // = 96;
	//do one-time initialization
	int out_index; // = -1;
	float32_t ring_max; // = 0.0;
	float32_t volts; // = 0.0;
	float32_t save_volts; // = 0.0;
	float32_t fast_backaverage; // = 0.0;
	float32_t hang_backaverage; // = 0.0;
	int hang_counter; // = 0;
	uint8_t decay_type; // = 0;
	uint8_t state; // = 0;
	int attack_buffsize;
	uint32_t in_index;
	float32_t attack_mult;
	float32_t decay_mult;
	float32_t fast_decay_mult;
	float32_t fast_backmult;
	float32_t onemfast_backmult;
	float32_t out_target;
	float32_t min_volts;
	float32_t inv_out_target;
	float32_t tmp;
	float32_t slope_constant;
	float32_t inv_max_input;
	float32_t hang_level;
	float32_t hang_backmult;
	float32_t onemhang_backmult;
	float32_t hang_decay_mult;
} agc_variables_t;

extern agc_variables_t agc_wdsp;
extern void SetupAgcWdsp(void);
extern void RxAgcWdsp(int16_t blockSize, float32_t *agcbuffer1);

#endif
