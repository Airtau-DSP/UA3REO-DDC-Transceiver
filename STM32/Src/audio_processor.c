#include "audio_processor.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include "fpga.h"
#include "trx_manager.h"
#include "wm8731.h"
#include "functions.h"
#include "audio_filters.h"
#include "agc.h"
#include "notch_filter.h"
#include "settings.h"

uint32_t AUDIOPROC_samples=0;
float32_t Audio_OUT_FloatBuffer[CODEC_AUDIO_BUFFER_SIZE]={0};

const uint16_t numBlocks=FPGA_AUDIO_BUFFER_SIZE/APROCESSOR_BLOCK_SIZE;
uint16_t block=0;

void initAudioProcessor(void)
{
	arm_fir_init_f32(&FIR_RX_Hilbert_I, IQ_HILBERT_TAPS, (float32_t *)&i_rx_3k6_coeffs, (float32_t *)&Fir_Rx_Hilbert_State_I[0], APROCESSOR_BLOCK_SIZE); // 0deg Hilbert 3.74 kHz
  arm_fir_init_f32(&FIR_RX_Hilbert_Q, IQ_HILBERT_TAPS, (float32_t *)&q_rx_3k6_coeffs, (float32_t *)&Fir_Rx_Hilbert_State_Q[0], APROCESSOR_BLOCK_SIZE); // -90deg Hilbert 3.74 kHz
	
	arm_fir_init_f32(&FIR_RX_LPF, FIR_LPF_Taps, (float32_t *)&FIR_2k7_LPF, (float32_t *)&Fir_Rx_LPF_State[0], APROCESSOR_BLOCK_SIZE); // LPF 2.7kHz
	
	arm_fill_f32(0.0,iir_rx_state,IIR_2k7_MAXnumStages+FPGA_AUDIO_BUFFER_SIZE);
	IIR_2k7_LPF.pState = iir_rx_state;
	
	arm_fill_f32(0.0,IIR_aa_state,IIR_aa_5k_numStages+FPGA_AUDIO_BUFFER_SIZE);
	IIR_aa_5k.pState = IIR_aa_state;
	
	//InitNotchFilter();
	SetupAgcWdsp(); //AGC
}

void processRxAudio(void)
{
	if(FPGA_Audio_IN_ActiveBuffer==0 && !FPGA_Audio_IN_Buffer_Full_A) return;
	if(FPGA_Audio_IN_ActiveBuffer==1 && !FPGA_Audio_IN_Buffer_Full_B) return;
	FPGA_busy=true;
	AUDIOPROC_samples++;
	float32_t *FPGA_Audio_IN_Buffer_Q;
	float32_t *FPGA_Audio_IN_Buffer_I;
	if(FPGA_Audio_IN_ActiveBuffer==0)
	{
		FPGA_Audio_IN_ActiveBuffer=1;
		FPGA_Audio_IN_Buffer_Q=&FPGA_Audio_IN_Buffer_Q_A[0];
		FPGA_Audio_IN_Buffer_I=&FPGA_Audio_IN_Buffer_I_A[0];
		FPGA_Audio_IN_Buffer_Full_A=false;
	}
	else if(FPGA_Audio_IN_ActiveBuffer==1)
	{
		FPGA_Audio_IN_ActiveBuffer=0;
		FPGA_Audio_IN_Buffer_Q=&FPGA_Audio_IN_Buffer_Q_B[0];
		FPGA_Audio_IN_Buffer_I=&FPGA_Audio_IN_Buffer_I_B[0];
		FPGA_Audio_IN_Buffer_Full_B=false;
	}
	FPGA_busy=false;
	//SSB
	if(TRX_getMode()==TRX_MODE_LSB || TRX_getMode()==TRX_MODE_USB)
	{
		for(block=0;block<numBlocks;block++)
    {
        arm_fir_f32(&FIR_RX_Hilbert_I,FPGA_Audio_IN_Buffer_I+(block*APROCESSOR_BLOCK_SIZE), FPGA_Audio_IN_Buffer_I+(block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
        arm_fir_f32(&FIR_RX_Hilbert_Q,FPGA_Audio_IN_Buffer_Q+(block*APROCESSOR_BLOCK_SIZE), FPGA_Audio_IN_Buffer_Q+(block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
    }
		switch(TRX_getMode())
		{
			case TRX_MODE_LSB:
				arm_sub_f32(FPGA_Audio_IN_Buffer_I, FPGA_Audio_IN_Buffer_Q, Audio_OUT_FloatBuffer, FPGA_AUDIO_BUFFER_SIZE);   // difference of I and Q - LSB
			break;
			case TRX_MODE_USB:
				arm_add_f32(FPGA_Audio_IN_Buffer_I, FPGA_Audio_IN_Buffer_Q, Audio_OUT_FloatBuffer, FPGA_AUDIO_BUFFER_SIZE);   // sum of I and Q - USB
			break;
		}
		
		for(block=0;block<numBlocks;block++)
		{
			arm_fir_f32(&FIR_RX_LPF,(float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE), (float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //FIR LPF
			if(TRX.Agc) RxAgcWdsp(APROCESSOR_BLOCK_SIZE, (float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE)); //AGC
			//NotchFilter(APROCESSOR_BLOCK_SIZE, (float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE));     //NotchFilter
			if(!TRX.Agc && TRX.Gain_level>1) arm_scale_f32((float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE),TRX.Gain_level, (float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //GAIN
			arm_biquad_cascade_df1_f32(&IIR_biquad, (float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE),(float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); // this is the biquad filter, a notch, peak, and lowshelf filter
			arm_iir_lattice_f32(&IIR_aa_5k, (float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE), (float32_t *)&Audio_OUT_FloatBuffer+(block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); // IIR ARMA-type lattice filter
		}
		
		if(CODEC_Audio_OUT_ActiveBuffer==0)
		{
			for(uint16_t i=0;i<FPGA_AUDIO_BUFFER_SIZE;i++)
			{
				CODEC_Audio_OUT_Buffer_A[i*2]=Audio_OUT_FloatBuffer[i];
				CODEC_Audio_OUT_Buffer_A[i*2+1]=Audio_OUT_FloatBuffer[i];
			}
		}
		else if(CODEC_Audio_OUT_ActiveBuffer==1)
		{
			for(uint16_t i=0;i<FPGA_AUDIO_BUFFER_SIZE;i++)
			{	
				CODEC_Audio_OUT_Buffer_B[i*2]=Audio_OUT_FloatBuffer[i];
				CODEC_Audio_OUT_Buffer_B[i*2+1]=Audio_OUT_FloatBuffer[i];
			}
		}
	}
	//IQ MODE
	else if(TRX_getMode()==TRX_MODE_IQ)
	{
		if(TRX.Gain_level>1) arm_scale_f32(FPGA_Audio_IN_Buffer_I,TRX.Gain_level, FPGA_Audio_IN_Buffer_I, FPGA_AUDIO_BUFFER_SIZE); //GAIN
		if(TRX.Gain_level>1) arm_scale_f32(FPGA_Audio_IN_Buffer_Q,TRX.Gain_level, FPGA_Audio_IN_Buffer_Q, FPGA_AUDIO_BUFFER_SIZE); //GAIN
		if(CODEC_Audio_OUT_ActiveBuffer==0)
		{
			for(uint16_t i=0;i<FPGA_AUDIO_BUFFER_SIZE;i++)
			{
				CODEC_Audio_OUT_Buffer_A[i*2]=FPGA_Audio_IN_Buffer_I[i];
				CODEC_Audio_OUT_Buffer_A[i*2+1]=FPGA_Audio_IN_Buffer_Q[i];
			}
		}
		else if(CODEC_Audio_OUT_ActiveBuffer==1)
		{
			for(uint16_t i=0;i<FPGA_AUDIO_BUFFER_SIZE;i++)
			{
				CODEC_Audio_OUT_Buffer_B[i*2]=FPGA_Audio_IN_Buffer_I[i];
				CODEC_Audio_OUT_Buffer_B[i*2+1]=FPGA_Audio_IN_Buffer_Q[i];
			}
		}
	}
	CODEC_Audio_OUT_Buffer_A_Length=FPGA_AUDIO_BUFFER_SIZE*2;
	CODEC_Audio_OUT_Buffer_B_Length=FPGA_AUDIO_BUFFER_SIZE*2;
	//logToUART1_str("-");
}
