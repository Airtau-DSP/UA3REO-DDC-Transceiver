#ifndef AUDIO_FILTERS_h
#define AUDIO_FILTERS_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"
#include "fpga.h"

#define IQ_RX_HILBERT_TAPS 89
#define IQ_TX_HILBERT_TAPS		201
#define FIR_LPF_Taps 89
#define IIR_2k7_MAXnumStages 14
#define IIR_aa_5k_numStages 6

#define FIR_RX_HILBERT_STATE_SIZE (IQ_RX_HILBERT_TAPS + FPGA_AUDIO_BUFFER_HALF_SIZE)
#define FIR_TX_HILBERT_STATE_SIZE (IQ_TX_HILBERT_TAPS + FPGA_AUDIO_BUFFER_HALF_SIZE)
#define FIR_RX_LPF_STATE_SIZE (IIR_2k7_MAXnumStages + FPGA_AUDIO_BUFFER_HALF_SIZE)
#define FIR_RX_AA_STATE_SIZE (IIR_aa_5k_numStages + FPGA_AUDIO_BUFFER_HALF_SIZE)

extern arm_fir_instance_f32    FIR_RX_Hilbert_I;
extern arm_fir_instance_f32    FIR_RX_Hilbert_Q;
extern arm_fir_instance_f32    FIR_TX_Hilbert_I;
extern arm_fir_instance_f32    FIR_TX_Hilbert_Q;
extern arm_fir_instance_f32    FIR_RX_LPF;
extern arm_iir_lattice_instance_f32 IIR_2k7_LPF;
extern arm_iir_lattice_instance_f32 IIR_2k7_BPF;
extern arm_iir_lattice_instance_f32 IIR_aa_5k;
extern arm_biquad_casd_df1_inst_f32 IIR_biquad;

extern float32_t    Fir_Rx_Hilbert_State_I[FIR_RX_HILBERT_STATE_SIZE];
extern float32_t    Fir_Rx_Hilbert_State_Q[FIR_RX_HILBERT_STATE_SIZE];
extern float32_t    Fir_Tx_Hilbert_State_I[FIR_TX_HILBERT_STATE_SIZE];
extern float32_t    Fir_Tx_Hilbert_State_Q[FIR_TX_HILBERT_STATE_SIZE];
extern float32_t		Fir_Rx_LPF_State[FPGA_AUDIO_BUFFER_SIZE+FIR_LPF_Taps];
extern float32_t		iir_rx_state[FIR_RX_LPF_STATE_SIZE];
extern float32_t		IIR_aa_state[FIR_RX_AA_STATE_SIZE];

extern const float32_t i_rx_3k6_coeffs[IQ_RX_HILBERT_TAPS];
extern const float32_t q_rx_3k6_coeffs[IQ_RX_HILBERT_TAPS];
extern const float32_t i_tx_coeffs[IQ_TX_HILBERT_TAPS];
extern const float32_t q_tx_coeffs[IQ_TX_HILBERT_TAPS];
extern const float32_t FIR_2k7_LPF[FIR_LPF_Taps];

#endif
