#include "notch_filter.h"
#include "stm32f4xx_hal.h"
#include "arm_math.h"
#include "audio_processor.h"

//FROM mcHF QRP Transceiver M0NKA
#define	DSP_NOTCH_NUMTAPS_MAX	128
#define	DSP_NOTCH_NUMTAPS_MIN		32
#define	DSP_NOTCH_NUMTAPS_DEFAULT	96
#define	DSP_NOTCH_BUFLEN_MIN	48		// minimum length of decorrelation buffer for the notch filter FIR
#define	DSP_NOTCH_BUFLEN_MAX	192 // maximum decorrelation buffer length for the notch filter FIR
#define LMS2_NOTCH_STATE_ARRAY_SIZE (DSP_NOTCH_NUMTAPS_MAX + APROCESSOR_BLOCK_SIZE)
#define	DSP_NOTCH_MU_DEFAULT	25 // default convergence setting for the notch
#define	DSP_NOTCH_DELAYBUF_DEFAULT	104 // default decorrelation buffer length for the notch filter FIR

unsigned long		lms2_inbuf = 0;
unsigned long		lms2_outbuf = 0;

typedef struct
{
	float32_t   errsig2[APROCESSOR_BLOCK_SIZE];
	arm_lms_norm_instance_f32	lms2Norm_instance;
	arm_lms_instance_f32	    lms2_instance;
	float32_t	                lms2StateF32[LMS2_NOTCH_STATE_ARRAY_SIZE];
	float32_t	                lms2NormCoeff_f32[DSP_NOTCH_NUMTAPS_MAX];
	float32_t	                lms2_nr_delay[DSP_NOTCH_BUFLEN_MAX];
} LMSData;

LMSData notch_lms_Data;

void InitNotchFilter(void)
{
	// AUTO NOTCH INIT START
// LMS instance 2 - Automatic Notch Filter
// Calculate "mu" (convergence rate) from user "Notch ConvRate" setting
	float32_t  mu_calc = log10f(((DSP_NOTCH_MU_DEFAULT + 1.0) / 1500.0) + 1.0);		// get user setting (0 = slowest)
	arm_fill_f32(0.0, notch_lms_Data.lms2StateF32, LMS2_NOTCH_STATE_ARRAY_SIZE);
	//arm_fill_f32(0.0,notch_lms_Data.lms2NormCoeff_f32,DSP_NOTCH_NUMTAPS_MAX);      // yes - zero coefficient buffers
	arm_lms_norm_init_f32(&notch_lms_Data.lms2Norm_instance, DSP_NOTCH_NUMTAPS_DEFAULT, notch_lms_Data.lms2NormCoeff_f32, notch_lms_Data.lms2StateF32, mu_calc, APROCESSOR_BLOCK_SIZE);
	arm_fill_f32(0.0, notch_lms_Data.lms2_nr_delay, DSP_NOTCH_BUFLEN_MAX);
	arm_fill_f32(0.0, notch_lms_Data.lms2NormCoeff_f32, DSP_NOTCH_NUMTAPS_MAX);      // yes - zero coefficient buffers
	// AUTO NOTCH INIT END
}

void NotchFilter(int16_t blockSize, float32_t *notchbuffer)
{
	// DSP Automatic Notch Filter using LMS (Least Mean Squared) algorithm
	arm_copy_f32(notchbuffer, &notch_lms_Data.lms2_nr_delay[lms2_inbuf], blockSize);	// put new data into the delay buffer
	arm_lms_norm_f32(&notch_lms_Data.lms2Norm_instance, notchbuffer, &notch_lms_Data.lms2_nr_delay[lms2_outbuf], notch_lms_Data.errsig2, notchbuffer, blockSize);	// do automatic notch
	// Desired (notched) audio comes from the "error" term - "errsig2" is used to hold the discarded ("non-error") audio data
	lms2_inbuf += blockSize;				// update circular de-correlation delay buffer
	lms2_outbuf = lms2_inbuf + blockSize;
	lms2_inbuf %= DSP_NOTCH_DELAYBUF_DEFAULT;
	lms2_outbuf %= DSP_NOTCH_DELAYBUF_DEFAULT;
	//
}
