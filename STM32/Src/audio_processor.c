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

float32_t Processor_AVG_amplitude = 0;
float32_t ampl_val_i=0;
float32_t ampl_val_q=0;

void initAudioProcessor(void)
{
	InitFilters();
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
		FPGA_Audio_Buffer_I_tmp[i] = (int16_t)Processor_AudioBuffer_A[i*2]/32767;
		FPGA_Audio_Buffer_Q_tmp[i] = (int16_t)Processor_AudioBuffer_A[i*2+1]/32767;
	}
	
	arm_scale_f32(FPGA_Audio_Buffer_I_tmp, TRX.MicGain_level*0.1, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //MIC GAIN
	arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, TRX.MicGain_level*0.1, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE); //MIC GAIN
	
	if (TRX_getMode() == TRX_MODE_LSB || TRX_getMode() == TRX_MODE_USB || TRX_getMode() == TRX_MODE_DIGI_L || TRX_getMode() == TRX_MODE_DIGI_U)
	{
		for (block = 0; block < numBlocks; block++)
			arm_iir_lattice_f32(&IIR_RX_SSB_LPF, (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE); //IIR LPF 2.9k
		
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
	readHalfFromCircleBuffer32((float32_t *)&FPGA_Audio_Buffer_Q[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], FPGA_Audio_Buffer_Index, FPGA_AUDIO_BUFFER_SIZE);
	readHalfFromCircleBuffer32((float32_t *)&FPGA_Audio_Buffer_I[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_Audio_Buffer_Index, FPGA_AUDIO_BUFFER_SIZE);
		
	//Anti-"click"
	for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		arm_abs_f32(&FPGA_Audio_Buffer_I_tmp[i],&ampl_val_i,1);
		arm_abs_f32(&FPGA_Audio_Buffer_Q_tmp[i],&ampl_val_q,1);
		if(ampl_val_i>Processor_AVG_amplitude) Processor_AVG_amplitude+=(float32_t)CLICK_REMOVE_STEPSIZE;
		if(ampl_val_i<Processor_AVG_amplitude) Processor_AVG_amplitude-=(float32_t)CLICK_REMOVE_STEPSIZE;
		if(ampl_val_q>Processor_AVG_amplitude) Processor_AVG_amplitude+=(float32_t)CLICK_REMOVE_STEPSIZE;
		if(ampl_val_q<Processor_AVG_amplitude) Processor_AVG_amplitude-=(float32_t)CLICK_REMOVE_STEPSIZE;
		if(ampl_val_i-Processor_AVG_amplitude>(float32_t)CLICK_REMOVE_THRESHOLD || ampl_val_q-Processor_AVG_amplitude>(float32_t)CLICK_REMOVE_THRESHOLD)
		{ 
			FPGA_Audio_Buffer_I_tmp[i]=0;
			FPGA_Audio_Buffer_Q_tmp[i]=0; 
		}
	}
	
	//SSB
	if (TRX_getMode() == TRX_MODE_LSB || TRX_getMode() == TRX_MODE_USB || TRX_getMode() == TRX_MODE_DIGI_L || TRX_getMode() == TRX_MODE_DIGI_U || TRX_getMode() == TRX_MODE_AM)
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
			case TRX_MODE_AM:
				for(int i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
            arm_sqrt_f32(FPGA_Audio_Buffer_I_tmp[i] * FPGA_Audio_Buffer_I_tmp[i] + FPGA_Audio_Buffer_Q_tmp[i] * FPGA_Audio_Buffer_Q_tmp[i], &FPGA_Audio_Buffer_I_tmp[i]);
				break;
		}
		
		for (block = 0; block < numBlocks; block++)
			arm_iir_lattice_f32(&IIR_RX_SSB_LPF, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE); //IIR LPF 2.9k
		for (block = 0; block < numBlocks; block++)
			if (TRX.Agc) RxAgcWdsp(APROCESSOR_BLOCK_SIZE, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE)); //AGC

		memcpy(&FPGA_Audio_Buffer_Q_tmp[0],&FPGA_Audio_Buffer_I_tmp[0],FPGA_AUDIO_BUFFER_HALF_SIZE*4); //double channel
	}
	
	//OUT Volume
	arm_scale_f32(FPGA_Audio_Buffer_I_tmp, (float32_t)TRX.Volume/(float32_t)100, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
	arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, (float32_t)TRX.Volume/(float32_t)100, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
	
	//Prepare data to DMA
	if (Processor_AudioBuffer_ReadyBuffer == 0)
	{
		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
		{
			Processor_AudioBuffer_B[i * 2] = FPGA_Audio_Buffer_I_tmp[i]*32767; //left channel
			Processor_AudioBuffer_B[i * 2 + 1] = FPGA_Audio_Buffer_Q_tmp[i]*32767; //right channel
		}
		Processor_AudioBuffer_ReadyBuffer = 1;
	}
	else
	{
		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
		{
			Processor_AudioBuffer_A[i * 2] = FPGA_Audio_Buffer_I_tmp[i]*32767; //left channel
			Processor_AudioBuffer_A[i * 2 + 1] = FPGA_Audio_Buffer_Q_tmp[i]*32767; //right channel
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
