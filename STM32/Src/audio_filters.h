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
#define IIR_RX_SSB_LPF_STAGES 11

#define FIR_RX_HILBERT_STATE_SIZE (IQ_RX_HILBERT_TAPS + FPGA_AUDIO_BUFFER_HALF_SIZE)
#define FIR_TX_HILBERT_STATE_SIZE (IQ_TX_HILBERT_TAPS + FPGA_AUDIO_BUFFER_HALF_SIZE)
#define IIR_RX_SSB_LPF_Taps_STATE_SIZE (FPGA_AUDIO_BUFFER_SIZE + IIR_RX_SSB_LPF_STAGES)

extern arm_fir_instance_f32    FIR_RX_Hilbert_I;
extern arm_fir_instance_f32    FIR_RX_Hilbert_Q;
extern arm_fir_instance_f32    FIR_TX_Hilbert_I;
extern arm_fir_instance_f32    FIR_TX_Hilbert_Q;
extern arm_fir_instance_f32    FIR_RX_SSB_LPF;
extern arm_iir_lattice_instance_f32 IIR_RX_SSB_LPF;

extern void InitFilters(void);

#endif
