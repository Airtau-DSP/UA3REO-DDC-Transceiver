#ifndef AUDIO_FILTERS_h
#define AUDIO_FILTERS_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"
#include "fpga.h"

#define IQ_TX_HILBERT_TAPS		201
#define IIR_LPF_CW_STAGES 7
#define IIR_LPF_STAGES 11
#define IIR_HPF_STAGES 6
#define IIR_HPF_SQL_STAGES 6
#define NOTCH_STAGES 1
#define NOTCH_COEFF_IN_STAGE 5

typedef enum {
	BIQUAD_onepolelp,
	BIQUAD_onepolehp,
	BIQUAD_lowpass,
	BIQUAD_highpass,
	BIQUAD_bandpass,
	BIQUAD_notch,
	BIQUAD_peak,
	BIQUAD_lowShelf,
	BIQUAD_highShelf
}  BIQUAD_TYPE; 

#define FIR_TX_HILBERT_STATE_SIZE (IQ_TX_HILBERT_TAPS + FPGA_AUDIO_BUFFER_HALF_SIZE - 1)
#define IIR_LPF_Taps_STATE_SIZE (FPGA_AUDIO_BUFFER_SIZE + IIR_LPF_STAGES)
#define IIR_HPF_Taps_STATE_SIZE (FPGA_AUDIO_BUFFER_SIZE + IIR_LPF_STAGES)
#define IIR_HPF_SQL_STATE_SIZE (FPGA_AUDIO_BUFFER_SIZE + IIR_HPF_SQL_STAGES)

extern arm_fir_instance_f32    FIR_TX_Hilbert_I;
extern arm_fir_instance_f32    FIR_TX_Hilbert_Q;
extern arm_iir_lattice_instance_f32 IIR_LPF_I;
extern arm_iir_lattice_instance_f32 IIR_LPF_Q;
extern arm_iir_lattice_instance_f32 IIR_HPF_I;
extern arm_iir_lattice_instance_f32 IIR_HPF_Q;
extern arm_iir_lattice_instance_f32 IIR_Squelch_HPF;
extern arm_biquad_cascade_df2T_instance_f32 NOTCH_FILTER;
extern volatile bool NeedReinitNotch;
	
extern void InitAudioFilters(void);
extern void ReinitAudioFilters(void);
extern void InitNotchFilter(void);
extern void dc_filter(float32_t *agcBuffer, int16_t blockSize, uint8_t stateNum);

//сохранение старых значений семплов для DC фильтра. Несколько состояний для разных потребителей
typedef struct {
	float32_t x_prev;
	float32_t y_prev;
} dc_filter_state_type;

#endif
