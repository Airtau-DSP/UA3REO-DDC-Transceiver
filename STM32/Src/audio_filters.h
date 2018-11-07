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
#define FIR_RX_SSB_LPF_Taps 89

#define FIR_RX_HILBERT_STATE_SIZE (IQ_RX_HILBERT_TAPS + FPGA_AUDIO_BUFFER_HALF_SIZE)
#define FIR_TX_HILBERT_STATE_SIZE (IQ_TX_HILBERT_TAPS + FPGA_AUDIO_BUFFER_HALF_SIZE)
#define FIR_RX_SSB_LPF_Taps_STATE_SIZE (FPGA_AUDIO_BUFFER_SIZE + FIR_RX_SSB_LPF_Taps)

extern arm_fir_instance_f32    FIR_RX_Hilbert_I;
extern arm_fir_instance_f32    FIR_RX_Hilbert_Q;
extern arm_fir_instance_f32    FIR_TX_Hilbert_I;
extern arm_fir_instance_f32    FIR_TX_Hilbert_Q;
extern arm_fir_instance_f32    FIR_RX_SSB_LPF;

extern float32_t    Fir_Rx_Hilbert_State_I[FIR_RX_HILBERT_STATE_SIZE];
extern float32_t    Fir_Rx_Hilbert_State_Q[FIR_RX_HILBERT_STATE_SIZE];
extern float32_t    Fir_Tx_Hilbert_State_I[FIR_TX_HILBERT_STATE_SIZE];
extern float32_t    Fir_Tx_Hilbert_State_Q[FIR_TX_HILBERT_STATE_SIZE];
extern float32_t		FIR_RX_SSB_LPF_State[FIR_RX_SSB_LPF_Taps_STATE_SIZE];

extern const float32_t i_rx_3k6_coeffs[IQ_RX_HILBERT_TAPS];
extern const float32_t q_rx_3k6_coeffs[IQ_RX_HILBERT_TAPS];
extern const float32_t i_tx_coeffs[IQ_TX_HILBERT_TAPS];
extern const float32_t q_tx_coeffs[IQ_TX_HILBERT_TAPS];
extern const float32_t FIR_RX_SSB_LPF_2k7_coeffs[FIR_RX_SSB_LPF_Taps];

#endif
