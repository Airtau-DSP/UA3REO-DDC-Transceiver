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
#include "settings.h"

uint32_t AUDIOPROC_samples = 0;
uint32_t AUDIOPROC_TXA_samples = 0;
uint32_t AUDIOPROC_TXB_samples = 0;
uint32_t Processor_AudioBuffer_A[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
uint32_t Processor_AudioBuffer_B[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
uint8_t Processor_AudioBuffer_ReadyBuffer = 0;
bool Processor_NeedBuffer = false;
float32_t FPGA_Audio_Buffer_Q_tmp[FPGA_AUDIO_BUFFER_HALF_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_I_tmp[FPGA_AUDIO_BUFFER_HALF_SIZE] = { 0 };
const uint16_t numBlocks = FPGA_AUDIO_BUFFER_HALF_SIZE / APROCESSOR_BLOCK_SIZE;
uint16_t block = 0;

uint32_t Processor_AVG_amplitude = 0;
int32_t ampl_val_i=0;
int32_t ampl_val_q=0;

void initAudioProcessor(void)
{
	arm_fir_init_f32(&FIR_RX_Hilbert_I, IQ_RX_HILBERT_TAPS, (float32_t *)&i_rx_3k6_coeffs, (float32_t *)&Fir_Rx_Hilbert_State_I[0], APROCESSOR_BLOCK_SIZE); // 0deg Hilbert 3.74 kHz
	arm_fir_init_f32(&FIR_RX_Hilbert_Q, IQ_RX_HILBERT_TAPS, (float32_t *)&q_rx_3k6_coeffs, (float32_t *)&Fir_Rx_Hilbert_State_Q[0], APROCESSOR_BLOCK_SIZE); // -90deg Hilbert 3.74 kHz
	arm_fir_init_f32(&FIR_TX_Hilbert_I, IQ_TX_HILBERT_TAPS, (float32_t *)&i_tx_coeffs, (float32_t *)&Fir_Tx_Hilbert_State_I[0], APROCESSOR_BLOCK_SIZE); // +/-45 degrees phase added
	arm_fir_init_f32(&FIR_TX_Hilbert_Q, IQ_TX_HILBERT_TAPS, (float32_t *)&q_tx_coeffs, (float32_t *)&Fir_Tx_Hilbert_State_Q[0], APROCESSOR_BLOCK_SIZE); // +/-45 degrees phase added

	arm_fir_init_f32(&FIR_RX_SSB_LPF, FIR_RX_SSB_LPF_Taps, (float32_t *)&FIR_RX_SSB_LPF_2k7_coeffs, (float32_t *)&FIR_RX_SSB_LPF_State[0], APROCESSOR_BLOCK_SIZE); // SSB LPF 2.7kHz

	SetupAgcWdsp(); //AGC
}

void processTxAudio(void)
{
	if(TRX_getMode()==TRX_MODE_LOOPBACK && HAL_DMA_GetState(&hdma_memtomem_dma2_stream0)==HAL_DMA_STATE_READY)
	{
		HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&CODEC_Audio_Buffer_TX[0], (uint32_t)&CODEC_Audio_Buffer_RX[FPGA_AUDIO_BUFFER_HALF_SIZE], CODEC_AUDIO_BUFFER_SIZE);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		return;
	}
	
	if (!Processor_NeedBuffer) return;

	readHalfFromCircleBufferU32((uint32_t *)&CODEC_Audio_Buffer_TX[0], (uint32_t *)&Processor_AudioBuffer_A[0], CODEC_AUDIO_BUFFER_SIZE - (__HAL_DMA_GET_COUNTER(&hdma_i2s3_ext_rx)/2), CODEC_AUDIO_BUFFER_SIZE);
	for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		FPGA_Audio_Buffer_I_tmp[i] = (int16_t)Processor_AudioBuffer_A[i*2];
		FPGA_Audio_Buffer_Q_tmp[i] = (int16_t)Processor_AudioBuffer_A[i*2+1];
	}
	
	arm_scale_f32(FPGA_Audio_Buffer_I_tmp, TRX.MicGain_level*0.1, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //MIC GAIN
	arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, TRX.MicGain_level*0.1, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //MIC GAIN
	
	if (TRX_getMode() == TRX_MODE_LSB || TRX_getMode() == TRX_MODE_USB || TRX_getMode() == TRX_MODE_DIGI_L || TRX_getMode() == TRX_MODE_DIGI_U)
	{
		for (block = 0; block < numBlocks; block++)
		{
			arm_fir_f32(&FIR_RX_SSB_LPF, (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE); //FIR LPF
		}
		
		switch (TRX_getMode())
		{
			case TRX_MODE_USB:
			case TRX_MODE_DIGI_U:
				for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
					FPGA_Audio_Buffer_Q_tmp[i] = -FPGA_Audio_Buffer_I_tmp[i];
			break;
			case TRX_MODE_LSB:
			case TRX_MODE_DIGI_L:
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
	}
	
	//Send to FPGA DMA
	if (FPGA_Audio_Buffer_State)
	{
		AUDIOPROC_TXA_samples++;
		HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&FPGA_Audio_Buffer_I_tmp[0], (uint32_t)&FPGA_Audio_Buffer_I[FPGA_AUDIO_BUFFER_HALF_SIZE], FPGA_AUDIO_BUFFER_HALF_SIZE);
		HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&FPGA_Audio_Buffer_Q_tmp[0], (uint32_t)&FPGA_Audio_Buffer_Q[FPGA_AUDIO_BUFFER_HALF_SIZE], FPGA_AUDIO_BUFFER_HALF_SIZE);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}
	else
	{
		AUDIOPROC_TXB_samples++;
		HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&FPGA_Audio_Buffer_I_tmp[0], (uint32_t)&FPGA_Audio_Buffer_I[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
		HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&FPGA_Audio_Buffer_Q_tmp[0], (uint32_t)&FPGA_Audio_Buffer_Q[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}
	Processor_NeedBuffer = false;
}

void processRxAudio(void)
{
	if (!Processor_NeedBuffer) return;
	AUDIOPROC_samples++;
	//logToUART1_num32(FPGA_Audio_Buffer_Index);
	readHalfFromCircleBuffer32((float32_t *)&FPGA_Audio_Buffer_Q[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], FPGA_Audio_Buffer_Index, FPGA_AUDIO_BUFFER_SIZE);
	readHalfFromCircleBuffer32((float32_t *)&FPGA_Audio_Buffer_I[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_Audio_Buffer_Index, FPGA_AUDIO_BUFFER_SIZE);
		
	//Anti-"click"
	for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		ampl_val_i=abs((int32_t)FPGA_Audio_Buffer_I_tmp[i]);
		ampl_val_q=abs((int32_t)FPGA_Audio_Buffer_Q_tmp[i]);
		if(ampl_val_i>Processor_AVG_amplitude) Processor_AVG_amplitude++;
		if(ampl_val_i<Processor_AVG_amplitude) Processor_AVG_amplitude--;
		if(ampl_val_q>Processor_AVG_amplitude) Processor_AVG_amplitude++;
		if(ampl_val_q<Processor_AVG_amplitude) Processor_AVG_amplitude--;
		if(FPGA_Audio_Buffer_I_tmp[i]-Processor_AVG_amplitude>CLICK_REMOVE_THRESHOLD || FPGA_Audio_Buffer_Q_tmp[i]-Processor_AVG_amplitude>CLICK_REMOVE_THRESHOLD)
		{ 
			FPGA_Audio_Buffer_I_tmp[i]=0;
			FPGA_Audio_Buffer_Q_tmp[i]=0; 
		}
	}
	
	if (TRX.Gain_level > 1) arm_scale_f32(FPGA_Audio_Buffer_I_tmp, TRX.Gain_level, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //GAIN
	if (TRX.Gain_level > 1) arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, TRX.Gain_level, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //GAIN
	
	//SSB
	if (TRX_getMode() == TRX_MODE_LSB || TRX_getMode() == TRX_MODE_USB || TRX_getMode() == TRX_MODE_DIGI_L || TRX_getMode() == TRX_MODE_DIGI_U)
	{
		for (block = 0; block < numBlocks; block++)
		{
			arm_fir_f32(&FIR_RX_Hilbert_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
			arm_fir_f32(&FIR_RX_Hilbert_Q, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //hilbert fir
		}
		switch (TRX_getMode())
		{
		case TRX_MODE_LSB:
		case TRX_MODE_DIGI_L:
			arm_sub_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);   // difference of I and Q - LSB
			break;
		case TRX_MODE_USB:
		case TRX_MODE_DIGI_U:
			arm_add_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);   // sum of I and Q - USB
			break;
		}
		for (block = 0; block < numBlocks; block++)
		{
			arm_fir_f32(&FIR_RX_SSB_LPF, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //FIR LPF
			if (TRX.Agc) RxAgcWdsp(APROCESSOR_BLOCK_SIZE, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE)); //AGC
			if (!TRX.Agc && TRX.Gain_level > 1) arm_scale_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), TRX.Gain_level, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
		}

		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			FPGA_Audio_Buffer_Q_tmp[i] = FPGA_Audio_Buffer_I_tmp[i]; //double channel
	}
	
	//Prepare data to DMA
	if (Processor_AudioBuffer_ReadyBuffer == 0)
	{
		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
		{
			Processor_AudioBuffer_B[i * 2] = FPGA_Audio_Buffer_I_tmp[i]; //left channel
			Processor_AudioBuffer_B[i * 2 + 1] = FPGA_Audio_Buffer_Q_tmp[i]; //right channel
		}
		Processor_AudioBuffer_ReadyBuffer = 1;
	}
	else
	{
		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
		{
			Processor_AudioBuffer_A[i * 2] = FPGA_Audio_Buffer_I_tmp[i]; //left channel
			Processor_AudioBuffer_A[i * 2 + 1] = FPGA_Audio_Buffer_Q_tmp[i]; //right channel
		}
		Processor_AudioBuffer_ReadyBuffer = 0;
	}
	//Send to Codec DMA
	if (WM8731_DMA_state) //compleate
	{
		if (Processor_AudioBuffer_ReadyBuffer == 0)
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer_RX[FPGA_AUDIO_BUFFER_SIZE], FPGA_AUDIO_BUFFER_SIZE);
		else
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&Processor_AudioBuffer_B[0], (uint32_t)&CODEC_Audio_Buffer_RX[FPGA_AUDIO_BUFFER_SIZE], FPGA_AUDIO_BUFFER_SIZE);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}
	else //half
	{
		if (Processor_AudioBuffer_ReadyBuffer == 0)
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer_RX[0], FPGA_AUDIO_BUFFER_SIZE);
		else
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&Processor_AudioBuffer_B[0], (uint32_t)&CODEC_Audio_Buffer_RX[0], FPGA_AUDIO_BUFFER_SIZE);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}
	Processor_NeedBuffer = false;
	//logToUART1_str("-");
}
