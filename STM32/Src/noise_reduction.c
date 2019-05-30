#include "noise_reduction.h"
#include "stm32f4xx_hal.h"
#include "arm_const_structs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include "arm_math.h"
#include "settings.h"
#include "functions.h"

static arm_lms_norm_instance_f32	lms2_Norm_instance;
static float32_t	                lms2_stateF32[NOISE_REDUCTION_TAPS + NOISE_REDUCTION_BLOCK_SIZE - 1];
static float32_t	                lms2_normCoeff_f32[NOISE_REDUCTION_TAPS];
static float32_t	                lms2_reference[NOISE_REDUCTION_REFERENCE_SIZE];
static float32_t   							lms2_errsig2[NOISE_REDUCTION_BLOCK_SIZE];

void InitNoiseReduction(void)
{
	arm_lms_norm_init_f32(&lms2_Norm_instance, NOISE_REDUCTION_TAPS, lms2_normCoeff_f32, lms2_stateF32, NOISE_REDUCTION_STEP, NOISE_REDUCTION_BLOCK_SIZE);
	arm_fill_f32(0.0f, lms2_reference, NOISE_REDUCTION_REFERENCE_SIZE);
	arm_fill_f32(0.0f, lms2_normCoeff_f32, NOISE_REDUCTION_TAPS);
}

void processNoiseReduction(float* bufferIn, float* bufferOut)
{
	if (!TRX.DNR) return;
	static uint16_t reference_index_old = 0;
	static uint16_t reference_index_new = 0;
	arm_copy_f32(bufferIn, &lms2_reference[reference_index_new], NOISE_REDUCTION_BLOCK_SIZE);
	arm_lms_norm_f32(&lms2_Norm_instance, bufferIn, &lms2_reference[reference_index_old], bufferOut, lms2_errsig2, NOISE_REDUCTION_BLOCK_SIZE);
	reference_index_old += NOISE_REDUCTION_BLOCK_SIZE;
	if (reference_index_old >= NOISE_REDUCTION_REFERENCE_SIZE) reference_index_old = 0;
	reference_index_new = reference_index_old + NOISE_REDUCTION_BLOCK_SIZE;
	if (reference_index_new >= NOISE_REDUCTION_REFERENCE_SIZE) reference_index_new = 0;
}
