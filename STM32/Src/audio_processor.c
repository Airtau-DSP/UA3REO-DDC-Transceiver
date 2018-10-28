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

uint32_t AUDIOPROC_samples = 0;
uint32_t AUDIOPROC_TXA_samples = 0;
uint32_t AUDIOPROC_TXB_samples = 0;
uint16_t Processor_AudioBuffer_A[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
uint16_t Processor_AudioBuffer_B[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
uint8_t Processor_AudioBuffer_ReadyBuffer = 0;
bool Processor_NeedBuffer = false;
float32_t FPGA_Audio_Buffer_Q_tmp[FPGA_AUDIO_BUFFER_HALF_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_I_tmp[FPGA_AUDIO_BUFFER_HALF_SIZE] = { 0 };

const uint16_t numBlocks = FPGA_AUDIO_BUFFER_HALF_SIZE / APROCESSOR_BLOCK_SIZE;
uint16_t block = 0;

uint16_t fpga_index_copy = 0;

void initAudioProcessor(void)
{
	arm_fir_init_f32(&FIR_RX_Hilbert_I, IQ_RX_HILBERT_TAPS, (float32_t *)&i_rx_3k6_coeffs, (float32_t *)&Fir_Rx_Hilbert_State_I[0], APROCESSOR_BLOCK_SIZE); // 0deg Hilbert 3.74 kHz
	arm_fir_init_f32(&FIR_RX_Hilbert_Q, IQ_RX_HILBERT_TAPS, (float32_t *)&q_rx_3k6_coeffs, (float32_t *)&Fir_Rx_Hilbert_State_Q[0], APROCESSOR_BLOCK_SIZE); // -90deg Hilbert 3.74 kHz
	arm_fir_init_f32(&FIR_TX_Hilbert_I, IQ_TX_HILBERT_TAPS, (float32_t *)&i_tx_coeffs, (float32_t *)&Fir_Tx_Hilbert_State_I[0], APROCESSOR_BLOCK_SIZE); //
	arm_fir_init_f32(&FIR_TX_Hilbert_Q, IQ_TX_HILBERT_TAPS, (float32_t *)&q_tx_coeffs, (float32_t *)&Fir_Tx_Hilbert_State_Q[0], APROCESSOR_BLOCK_SIZE); //

	arm_fir_init_f32(&FIR_RX_LPF, FIR_LPF_Taps, (float32_t *)&FIR_2k7_LPF, (float32_t *)&Fir_Rx_LPF_State[0], APROCESSOR_BLOCK_SIZE); // LPF 2.7kHz

	arm_fill_f32(0.0, iir_rx_state, IIR_2k7_MAXnumStages + FPGA_AUDIO_BUFFER_SIZE);
	IIR_2k7_LPF.pState = iir_rx_state;

	arm_fill_f32(0.0, IIR_aa_state, IIR_aa_5k_numStages + FPGA_AUDIO_BUFFER_SIZE);
	IIR_aa_5k.pState = IIR_aa_state;

	//InitNotchFilter();
	SetupAgcWdsp(); //AGC
}

void processTxAudio(void)
{
	if (!Processor_NeedBuffer) return;
	
	if (TRX_getMode() == TRX_MODE_LSB || TRX_getMode() == TRX_MODE_USB)
	{
		readHalfFromCircleBuffer16((uint16_t *)&CODEC_Audio_Buffer_TX[0], (uint16_t *)&Processor_AudioBuffer_A[0], CODEC_AUDIO_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_i2s3_ext_rx), CODEC_AUDIO_BUFFER_SIZE);
		
		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
		{
			FPGA_Audio_Buffer_I_tmp[i] = (int16_t)Processor_AudioBuffer_A[i*2];
			if(FPGA_Audio_Buffer_I_tmp[i]>0 && FPGA_Audio_Buffer_I_tmp[i]<10) FPGA_Audio_Buffer_I_tmp[i] = 0;
			if(FPGA_Audio_Buffer_I_tmp[i]<0 && FPGA_Audio_Buffer_I_tmp[i]>-10) FPGA_Audio_Buffer_I_tmp[i] = 0;
		}
		
		arm_scale_f32(FPGA_Audio_Buffer_I_tmp, TRX.MicGain_level*0.1, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //GAIN *0.01
		
		for (block = 0; block < numBlocks; block++)
		{
			arm_fir_f32(&FIR_RX_LPF, (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE); //FIR LPF
		}
		
		switch (TRX_getMode())
		{
			case TRX_MODE_USB:
				for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
					FPGA_Audio_Buffer_Q_tmp[i] = -FPGA_Audio_Buffer_I_tmp[i];
			break;
			case TRX_MODE_LSB:
				for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
					FPGA_Audio_Buffer_Q_tmp[i] = FPGA_Audio_Buffer_I_tmp[i];
			break;
		}
		
		for (block = 0; block < numBlocks; block++)
		{
			// + 45 deg to I data
			arm_fir_f32(&FIR_TX_Hilbert_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
			// - 45 deg to Q data
			arm_fir_f32(&FIR_TX_Hilbert_Q, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
		}

		if (FPGA_Audio_Buffer_State)
		{
			AUDIOPROC_TXA_samples++;
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&FPGA_Audio_Buffer_I_tmp[0], (uint32_t)&FPGA_Audio_Buffer_I[FPGA_AUDIO_BUFFER_HALF_SIZE], sizeof(FPGA_Audio_Buffer_I_tmp)/4);
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&FPGA_Audio_Buffer_Q_tmp[0], (uint32_t)&FPGA_Audio_Buffer_Q[FPGA_AUDIO_BUFFER_HALF_SIZE], sizeof(FPGA_Audio_Buffer_I_tmp)/4);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		}
		else
		{
			AUDIOPROC_TXB_samples++;
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&FPGA_Audio_Buffer_I_tmp[0], (uint32_t)&FPGA_Audio_Buffer_I[0], sizeof(FPGA_Audio_Buffer_I_tmp)/4);
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&FPGA_Audio_Buffer_Q_tmp[0], (uint32_t)&FPGA_Audio_Buffer_Q[0], sizeof(FPGA_Audio_Buffer_I_tmp)/4);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		}
	}
	Processor_NeedBuffer = false;
}

void processRxAudio(void)
{
	if (!Processor_NeedBuffer) return;
	AUDIOPROC_samples++;
	
	readHalfFromCircleBuffer32((float32_t *)&FPGA_Audio_Buffer_Q[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], FPGA_Audio_Buffer_Index, FPGA_AUDIO_BUFFER_SIZE);
	readHalfFromCircleBuffer32((float32_t *)&FPGA_Audio_Buffer_I[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_Audio_Buffer_Index, FPGA_AUDIO_BUFFER_SIZE);
		
	//SSB
	if (TRX_getMode() == TRX_MODE_LSB || TRX_getMode() == TRX_MODE_USB)
	{
		for (block = 0; block < numBlocks; block++)
		{
			arm_fir_f32(&FIR_RX_Hilbert_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
			arm_fir_f32(&FIR_RX_Hilbert_Q, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
		}
		switch (TRX_getMode())
		{
		case TRX_MODE_LSB:
			arm_sub_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);   // difference of I and Q - LSB
			break;
		case TRX_MODE_USB:
			arm_add_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);   // sum of I and Q - USB
			break;
		}

		for (block = 0; block < numBlocks; block++)
		{
			arm_fir_f32(&FIR_RX_LPF, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //FIR LPF
			if (TRX.Agc) RxAgcWdsp(APROCESSOR_BLOCK_SIZE, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE)); //AGC
			//NotchFilter(APROCESSOR_BLOCK_SIZE, (float32_t *)&FPGA_Audio_Buffer_I+(block*APROCESSOR_BLOCK_SIZE));     //NotchFilter
			if (!TRX.Agc && TRX.Gain_level > 1) arm_scale_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), TRX.Gain_level, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //GAIN
			arm_biquad_cascade_df1_f32(&IIR_biquad, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); // this is the biquad filter, a notch, peak, and lowshelf filter
			arm_iir_lattice_f32(&IIR_aa_5k, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); // IIR ARMA-type lattice filter
		}

		if (Processor_AudioBuffer_ReadyBuffer == 0)
		{
			for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			{
				Processor_AudioBuffer_B[i * 2] = FPGA_Audio_Buffer_I_tmp[i];
				Processor_AudioBuffer_B[i * 2 + 1] = FPGA_Audio_Buffer_I_tmp[i];
			}
			Processor_AudioBuffer_ReadyBuffer = 1;
		}
		else
		{
			for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			{
				Processor_AudioBuffer_A[i * 2] = FPGA_Audio_Buffer_I_tmp[i];
				Processor_AudioBuffer_A[i * 2 + 1] = FPGA_Audio_Buffer_I_tmp[i];
			}
			Processor_AudioBuffer_ReadyBuffer = 0;
		}
	}
	//IQ MODE
	else if (TRX_getMode() == TRX_MODE_IQ)
	{
		if (TRX.Gain_level > 1) arm_scale_f32(FPGA_Audio_Buffer_I_tmp, TRX.Gain_level, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //GAIN
		if (TRX.Gain_level > 1) arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, TRX.Gain_level, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //GAIN

		if (Processor_AudioBuffer_ReadyBuffer == 0)
		{
			for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			{
				Processor_AudioBuffer_B[i * 2] = FPGA_Audio_Buffer_I_tmp[i];
				Processor_AudioBuffer_B[i * 2 + 1] = FPGA_Audio_Buffer_Q_tmp[i];
			}
			Processor_AudioBuffer_ReadyBuffer = 1;
		}
		else
		{
			for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			{
				Processor_AudioBuffer_A[i * 2] = FPGA_Audio_Buffer_I_tmp[i];
				Processor_AudioBuffer_A[i * 2 + 1] = FPGA_Audio_Buffer_Q_tmp[i];
			}
			Processor_AudioBuffer_ReadyBuffer = 0;
		}
	}
	//Send to Codec DMA
	if (WM8731_DMA_state) //compleate
	{
		if (Processor_AudioBuffer_ReadyBuffer == 0)
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer[FPGA_AUDIO_BUFFER_SIZE], sizeof(Processor_AudioBuffer_A) / 4);
		else
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&Processor_AudioBuffer_B[0], (uint32_t)&CODEC_Audio_Buffer[FPGA_AUDIO_BUFFER_SIZE], sizeof(Processor_AudioBuffer_B) / 4);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}
	else //half
	{
		if (Processor_AudioBuffer_ReadyBuffer == 0)
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer[0], sizeof(Processor_AudioBuffer_A) / 4);
		else
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&Processor_AudioBuffer_B[0], (uint32_t)&CODEC_Audio_Buffer[0], sizeof(Processor_AudioBuffer_B) / 4);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}
	Processor_NeedBuffer = false;
	//logToUART1_str("-");
}
