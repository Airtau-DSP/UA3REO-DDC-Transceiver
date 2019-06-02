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
#include "profiler.h"
#include "usbd_audio_if.h"
#include "noise_reduction.h"
#include "cw_decoder.h"

volatile uint32_t AUDIOPROC_samples = 0;
volatile uint32_t AUDIOPROC_TXA_samples = 0;
volatile uint32_t AUDIOPROC_TXB_samples = 0;
int32_t Processor_AudioBuffer_A[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
int32_t Processor_AudioBuffer_B[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
volatile uint8_t Processor_AudioBuffer_ReadyBuffer = 0;
volatile bool Processor_NeedRXBuffer = false;
volatile bool Processor_NeedTXBuffer = false;
float32_t FPGA_Audio_Buffer_Q_tmp[FPGA_AUDIO_BUFFER_HALF_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_I_tmp[FPGA_AUDIO_BUFFER_HALF_SIZE] = { 0 };
volatile float32_t Processor_AVG_amplitude = 0.0f; //средняя амплитуда семплов при прёме
volatile float32_t Processor_TX_MAX_amplitude = 0.0f; //средняя амплитуда семплов при передаче
volatile float32_t Processor_RX_Audio_Samples_MAX_value = 0.0f; //максимальное значение семплов
volatile float32_t Processor_RX_Audio_Samples_MIN_value = 0.0f; //минимальное значение семплов
volatile float32_t ALC_need_gain = 1.0f;
volatile float32_t fm_sql_avg = 0.0f;

static const uint16_t numBlocks = FPGA_AUDIO_BUFFER_HALF_SIZE / APROCESSOR_BLOCK_SIZE;
static float32_t ampl_val_i = 0.0f;
static float32_t ampl_val_q = 0.0f;
static float32_t selected_rfpower_amplitude = 0.0f;
static float32_t ALC_need_gain_target = 1.0f;
static uint16_t block = 0;

static void doRX_LPF(void);
static void doRX_HPF(void);
static void doRX_DNR(void);
static void doRX_AGC(void);
static void doRX_NOTCH(void);
static void doRX_SMETER(void);
static void doRX_COPYCHANNEL(void);
static void DemodulateFM(void);
static void ModulateFM(void);

void initAudioProcessor(void)
{
	InitAudioFilters();
	InitAGC();
}

void processTxAudio(void)
{
	if (!Processor_NeedTXBuffer) return;
	AUDIOPROC_samples++;
	selected_rfpower_amplitude = TRX.RF_Power / 100.0f * MAX_TX_AMPLITUDE;

	if (TRX.InputType == 2) //USB AUDIO
	{
		uint16_t buffer_index = USB_AUDIO_GetTXBufferIndex_FS() / 2; //buffer 8bit, data 16 bit
		if ((buffer_index % 2) == 1) buffer_index--;
		readHalfFromCircleUSBBuffer((int16_t *)&USB_AUDIO_tx_buffer[0], (int32_t *)&Processor_AudioBuffer_A[0], buffer_index, USB_AUDIO_TX_BUFFER_SIZE / 2);
	}
	else //AUDIO CODEC AUDIO
	{
		uint16_t dma_index = __HAL_DMA_GET_COUNTER(hi2s3.hdmatx) / 2;
		if ((dma_index % 2) == 1) dma_index--;
		readHalfFromCircleBuffer32((uint32_t *)&CODEC_Audio_Buffer_TX[0], (uint32_t *)&Processor_AudioBuffer_A[0], dma_index, CODEC_AUDIO_BUFFER_SIZE);
	}

	for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		if (TRX_tune)
		{
			FPGA_Audio_Buffer_Q_tmp[i] = TUNE_AMPLITUDE;
			FPGA_Audio_Buffer_I_tmp[i] = TUNE_AMPLITUDE;
		}
		else
		{
			FPGA_Audio_Buffer_I_tmp[i] = (int16_t)(Processor_AudioBuffer_A[i * 2]);
			FPGA_Audio_Buffer_Q_tmp[i] = (int16_t)(Processor_AudioBuffer_A[i * 2 + 1]);
			//Process DC corrector filter
			dc_filter(FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE, 2);
			dc_filter(FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE, 3);
		}
	}

	if (TRX_getMode() != TRX_MODE_IQ && !TRX_tune)
	{
		//IIR HPF
		for (block = 0; block < numBlocks; block++)
			arm_iir_lattice_f32(&IIR_HPF_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE);
		//IIR LPF
		if (CurrentVFO()->Filter_Width > 0)
			for (block = 0; block < numBlocks; block++)
				arm_iir_lattice_f32(&IIR_LPF_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE);
		memcpy(&FPGA_Audio_Buffer_Q_tmp[0], &FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE * 4); //double left and right channel

		//RF PowerControl (Audio Level Control) Compressor
		Processor_TX_MAX_amplitude = 0;
		//ищем максимум в амплитуде
		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
		{
			arm_abs_f32(&FPGA_Audio_Buffer_I_tmp[i], &ampl_val_i, 1);
			arm_abs_f32(&FPGA_Audio_Buffer_Q_tmp[i], &ampl_val_q, 1);
			if (ampl_val_i > Processor_TX_MAX_amplitude) Processor_TX_MAX_amplitude = ampl_val_i;
			if (ampl_val_q > Processor_TX_MAX_amplitude) Processor_TX_MAX_amplitude = ampl_val_q;
		}
		if (Processor_TX_MAX_amplitude == 0.0f) Processor_TX_MAX_amplitude = 0.001f;
		//расчитываем целевое значение усиления
		ALC_need_gain_target = selected_rfpower_amplitude / Processor_TX_MAX_amplitude;
		//двигаем усиление на шаг
		if (ALC_need_gain_target > ALC_need_gain)
			ALC_need_gain += (ALC_need_gain_target - ALC_need_gain) / TX_AGC_STEPSIZE;
		else
			ALC_need_gain -= (ALC_need_gain - ALC_need_gain_target) / TX_AGC_STEPSIZE;

		if (ALC_need_gain_target < ALC_need_gain)
			ALC_need_gain = ALC_need_gain_target;
		if (ALC_need_gain < 0.0f) ALC_need_gain = 0.0f;
		//перегрузка (клиппинг), резко снижаем усиление
		if ((ALC_need_gain*Processor_TX_MAX_amplitude) > (selected_rfpower_amplitude*1.1f))
			ALC_need_gain = ALC_need_gain_target;
		if (ALC_need_gain > TX_AGC_MAXGAIN) ALC_need_gain = TX_AGC_MAXGAIN;
		//шумовой порог
		if (Processor_TX_MAX_amplitude < TX_AGC_NOISEGATE) ALC_need_gain = 0.0f;
		//оключаем усиление для некоторых видов мод
		if ((ALC_need_gain > 1.0f) && (TRX_getMode() == TRX_MODE_DIGI_L || TRX_getMode() == TRX_MODE_DIGI_U || TRX_getMode() == TRX_MODE_IQ || TRX_getMode() == TRX_MODE_LOOPBACK)) ALC_need_gain = 1.0f;
		if (TRX_tune) ALC_need_gain = 1.0f;
		//применяем усиление
		arm_scale_f32(FPGA_Audio_Buffer_I_tmp, ALC_need_gain, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
		arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, ALC_need_gain, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
		//
		switch (TRX_getMode())
		{
		case TRX_MODE_CW_L:
		case TRX_MODE_CW_U:
			if (!TRX_key_serial && !TRX_ptt_hard && !TRX_key_hard) selected_rfpower_amplitude = 0;
			for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			{
				FPGA_Audio_Buffer_Q_tmp[i] = selected_rfpower_amplitude;
				FPGA_Audio_Buffer_I_tmp[i] = selected_rfpower_amplitude;
			}
			break;
		case TRX_MODE_USB:
		case TRX_MODE_DIGI_U:
			for (block = 0; block < numBlocks; block++)
			{
				// + 45 deg to Q data
				arm_fir_f32(&FIR_TX_Hilbert_Q, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
				// - 45 deg to I data
				arm_fir_f32(&FIR_TX_Hilbert_I, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
			}
			break;
		case TRX_MODE_LSB:
		case TRX_MODE_DIGI_L:
			//hilbert fir
			for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
				FPGA_Audio_Buffer_Q_tmp[i] = FPGA_Audio_Buffer_I_tmp[i];
			for (block = 0; block < numBlocks; block++)
			{
				// + 45 deg to I data
				arm_fir_f32(&FIR_TX_Hilbert_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
				// - 45 deg to Q data
				arm_fir_f32(&FIR_TX_Hilbert_Q, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
			}
			break;
		case TRX_MODE_AM:
			for (block = 0; block < numBlocks; block++)
			{
				// + 45 deg to I data
				arm_fir_f32(&FIR_TX_Hilbert_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
				// - 45 deg to Q data
				arm_fir_f32(&FIR_TX_Hilbert_Q, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
			}
			for (size_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			{
				float32_t i_am = ((FPGA_Audio_Buffer_I_tmp[i] - FPGA_Audio_Buffer_Q_tmp[i]) + (selected_rfpower_amplitude)) / 2.0f;
				float32_t q_am = ((FPGA_Audio_Buffer_Q_tmp[i] - FPGA_Audio_Buffer_I_tmp[i]) - (selected_rfpower_amplitude)) / 2.0f;
				FPGA_Audio_Buffer_I_tmp[i] = i_am;
				FPGA_Audio_Buffer_Q_tmp[i] = q_am;
			}
			break;
		case TRX_MODE_NFM:
		case TRX_MODE_WFM:
			ModulateFM();
			break;
		default:
			break;
		}
	}

	if (TRX.Mute && !TRX_tune)
	{
		arm_scale_f32(FPGA_Audio_Buffer_I_tmp, 0, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
		arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, 0, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
	}

	//Send TX data to FFT
	for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		if (NeedFFTInputBuffer)
		{
			FFTInput_I[FFT_buff_index] = FPGA_Audio_Buffer_I_tmp[i];
			FFTInput_Q[FFT_buff_index] = FPGA_Audio_Buffer_Q_tmp[i];
			FFT_buff_index++;
			if (FFT_buff_index == FFT_SIZE)
			{
				FFT_buff_index = 0;
				NeedFFTInputBuffer = false;
			}
		}
	}

	//Loopback mode
	if (TRX_getMode() == TRX_MODE_LOOPBACK && !TRX_tune)
	{
		//OUT Volume
		arm_scale_f32(FPGA_Audio_Buffer_I_tmp, (float32_t)TRX.Volume / 50.0f, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);

		for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
		{
			Processor_AudioBuffer_A[i * 2] = FPGA_Audio_Buffer_I_tmp[i]; //left channel 
			Processor_AudioBuffer_A[i * 2 + 1] = Processor_AudioBuffer_A[i * 2]; //right channel 
		}

		if (WM8731_DMA_state) //compleate
		{
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer_RX[FPGA_AUDIO_BUFFER_SIZE], FPGA_AUDIO_BUFFER_SIZE);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
			AUDIOPROC_TXA_samples++;
		}
		else //half
		{
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer_RX[0], FPGA_AUDIO_BUFFER_SIZE);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
			AUDIOPROC_TXB_samples++;
		}
	}
	else
	{
		if (FPGA_Audio_Buffer_State) //Send to FPGA DMA
		{
			AUDIOPROC_TXA_samples++;
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&FPGA_Audio_Buffer_I_tmp[0], (uint32_t)&FPGA_Audio_SendBuffer_I[FPGA_AUDIO_BUFFER_HALF_SIZE], FPGA_AUDIO_BUFFER_HALF_SIZE);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
			HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&FPGA_Audio_Buffer_Q_tmp[0], (uint32_t)&FPGA_Audio_SendBuffer_Q[FPGA_AUDIO_BUFFER_HALF_SIZE], FPGA_AUDIO_BUFFER_HALF_SIZE);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		}
		else
		{
			AUDIOPROC_TXB_samples++;
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&FPGA_Audio_Buffer_I_tmp[0], (uint32_t)&FPGA_Audio_SendBuffer_I[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
			HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)&FPGA_Audio_Buffer_Q_tmp[0], (uint32_t)&FPGA_Audio_SendBuffer_Q[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
			HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		}
	}
	Processor_NeedTXBuffer = false;
	Processor_NeedRXBuffer = false;
}

void processRxAudio(void)
{
	if (!Processor_NeedRXBuffer) return;
	AUDIOPROC_samples++;
	uint16_t FPGA_Audio_Buffer_Index_tmp = FPGA_Audio_Buffer_Index;
	if (FPGA_Audio_Buffer_Index_tmp == 0)
		FPGA_Audio_Buffer_Index_tmp = FPGA_AUDIO_BUFFER_SIZE;
	else
		FPGA_Audio_Buffer_Index_tmp--;

	switch (TRX_getMode())
	{
	case TRX_MODE_IQ:
	case TRX_MODE_NFM:
	case TRX_MODE_WFM:
	case TRX_MODE_AM:
	case TRX_MODE_LOOPBACK:
		readHalfFromCircleBuffer32((uint32_t *)&FPGA_Audio_Buffer_SPEC_Q[0], (uint32_t *)&FPGA_Audio_Buffer_Q_tmp[0], FPGA_Audio_Buffer_Index_tmp, FPGA_AUDIO_BUFFER_SIZE);
		readHalfFromCircleBuffer32((uint32_t *)&FPGA_Audio_Buffer_SPEC_I[0], (uint32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_Audio_Buffer_Index_tmp, FPGA_AUDIO_BUFFER_SIZE);
		break;
	default:
		readHalfFromCircleBuffer32((uint32_t *)&FPGA_Audio_Buffer_VOICE_Q[0], (uint32_t *)&FPGA_Audio_Buffer_Q_tmp[0], FPGA_Audio_Buffer_Index_tmp, FPGA_AUDIO_BUFFER_SIZE);
		readHalfFromCircleBuffer32((uint32_t *)&FPGA_Audio_Buffer_VOICE_I[0], (uint32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_Audio_Buffer_Index_tmp, FPGA_AUDIO_BUFFER_SIZE);
		break;
	}

	//Process DC corrector filter
	dc_filter(FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE, 0);
	dc_filter(FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE, 1);

	//RF Gain
	arm_scale_f32(FPGA_Audio_Buffer_I_tmp, TRX.RF_Gain, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
	arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, TRX.RF_Gain, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);

	switch (TRX_getMode())
	{
	case TRX_MODE_LSB:
	case TRX_MODE_CW_L:
		doRX_HPF();
	case TRX_MODE_DIGI_L:
		doRX_LPF();
		arm_sub_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);   // difference of I and Q - LSB
		doRX_NOTCH();
		doRX_SMETER();
		doRX_DNR();
		doRX_AGC();
		if(TRX_getMode()==TRX_MODE_CW_L)
			CWDecoder_Process((float32_t *)&FPGA_Audio_Buffer_I_tmp[0]);
		doRX_COPYCHANNEL();
		break;
	case TRX_MODE_USB:
	case TRX_MODE_CW_U:
		doRX_HPF();
	case TRX_MODE_DIGI_U:
		doRX_LPF();
		arm_add_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);   // sum of I and Q - USB
		doRX_NOTCH();
		doRX_SMETER();
		doRX_DNR();
		doRX_AGC();
		if(TRX_getMode()==TRX_MODE_CW_U)
			CWDecoder_Process((float32_t *)&FPGA_Audio_Buffer_I_tmp[0]);
		doRX_COPYCHANNEL();
		break;
	case TRX_MODE_AM:
		doRX_LPF();
		arm_mult_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
		arm_mult_f32((float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
		arm_add_f32((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0], (float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
		for (int i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
			arm_sqrt_f32(FPGA_Audio_Buffer_I_tmp[i], &FPGA_Audio_Buffer_I_tmp[i]);
		doRX_NOTCH();
		doRX_SMETER();
		doRX_DNR();
		doRX_AGC();
		doRX_COPYCHANNEL();
		break;
	case TRX_MODE_NFM:
	case TRX_MODE_WFM:
		doRX_LPF();
		DemodulateFM();
		doRX_SMETER();
		doRX_DNR();
		doRX_AGC();
		doRX_COPYCHANNEL();
		break;
	case TRX_MODE_IQ:
	case TRX_MODE_LOOPBACK:
	default:
		doRX_SMETER();
		break;
	}

	//OUT Volume
	if (TRX.Mute)
	{
		arm_scale_f32(FPGA_Audio_Buffer_I_tmp, 0, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
		arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, 0, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
	}
	else
	{
		arm_scale_f32(FPGA_Audio_Buffer_I_tmp, (float32_t)TRX.Volume / 100.0f, FPGA_Audio_Buffer_I_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
		arm_scale_f32(FPGA_Audio_Buffer_Q_tmp, (float32_t)TRX.Volume / 100.0f, FPGA_Audio_Buffer_Q_tmp, FPGA_AUDIO_BUFFER_HALF_SIZE);
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
	if (WM8731_DMA_state) //complete
	{
		if (Processor_AudioBuffer_ReadyBuffer == 0)
			HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream0, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer_RX[FPGA_AUDIO_BUFFER_SIZE], FPGA_AUDIO_BUFFER_SIZE);
		else
			HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream0, (uint32_t)&Processor_AudioBuffer_B[0], (uint32_t)&CODEC_Audio_Buffer_RX[FPGA_AUDIO_BUFFER_SIZE], FPGA_AUDIO_BUFFER_SIZE);
		AUDIOPROC_TXA_samples++;
	}
	else //half
	{
		if (Processor_AudioBuffer_ReadyBuffer == 0)
			HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream1, (uint32_t)&Processor_AudioBuffer_A[0], (uint32_t)&CODEC_Audio_Buffer_RX[0], FPGA_AUDIO_BUFFER_SIZE);
		else
			HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream1, (uint32_t)&Processor_AudioBuffer_B[0], (uint32_t)&CODEC_Audio_Buffer_RX[0], FPGA_AUDIO_BUFFER_SIZE);
		AUDIOPROC_TXB_samples++;
	}

	//Send to USB Audio
	if (USB_AUDIO_need_rx_buffer)
	{
		if (Processor_AudioBuffer_ReadyBuffer == 0)
		{
			if (!USB_AUDIO_current_rx_buffer)
				for (uint16_t i = 0; i < (USB_AUDIO_RX_BUFFER_SIZE / 2); i++) USB_AUDIO_rx_buffer_a[i] = (int16_t)((int32_t)Processor_AudioBuffer_A[i] * (1.0f / TRX.Volume*100.0f));
			else
				for (uint16_t i = 0; i < (USB_AUDIO_RX_BUFFER_SIZE / 2); i++) USB_AUDIO_rx_buffer_b[i] = (int16_t)((int32_t)Processor_AudioBuffer_A[i] * (1.0f / TRX.Volume*100.0f));
		}
		else
		{
			if (!USB_AUDIO_current_rx_buffer)
				for (uint16_t i = 0; i < (USB_AUDIO_RX_BUFFER_SIZE / 2); i++) USB_AUDIO_rx_buffer_a[i] = (int16_t)((int32_t)Processor_AudioBuffer_B[i] * (1.0f / TRX.Volume*100.0f));
			else
				for (uint16_t i = 0; i < (USB_AUDIO_RX_BUFFER_SIZE / 2); i++) USB_AUDIO_rx_buffer_b[i] = (int16_t)((int32_t)Processor_AudioBuffer_B[i] * (1.0f / TRX.Volume*100.0f));
		}
		USB_AUDIO_need_rx_buffer = false;
	}
	//
	Processor_NeedRXBuffer = false;
}

static void doRX_LPF(void)
{
	//IIR LPF
	if (CurrentVFO()->Filter_Width > 0)
		for (block = 0; block < numBlocks; block++)
		{
			arm_iir_lattice_f32(&IIR_LPF_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
			arm_iir_lattice_f32(&IIR_LPF_Q, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_Q_tmp[0] + (block*APROCESSOR_BLOCK_SIZE), APROCESSOR_BLOCK_SIZE);
		}
}

static void doRX_HPF(void)
{
	//IIR HPF
	for (block = 0; block < numBlocks; block++)
	{
		arm_iir_lattice_f32(&IIR_HPF_I, (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE);
		arm_iir_lattice_f32(&IIR_HPF_Q, (float32_t *)&FPGA_Audio_Buffer_Q_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_Q_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE);
	}
}

static void doRX_NOTCH(void)
{
	if (TRX.NotchFilter)
	{
		for (block = 0; block < numBlocks; block++)
			arm_biquad_cascade_df2T_f32(&NOTCH_FILTER, (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], (float32_t *)&FPGA_Audio_Buffer_I_tmp[block*APROCESSOR_BLOCK_SIZE], APROCESSOR_BLOCK_SIZE);
	}
}

static void doRX_DNR(void)
{
	//Digital Noise Reduction
	if (TRX.DNR > 0)
	{
		for (block = 0; block < (FPGA_AUDIO_BUFFER_HALF_SIZE / NOISE_REDUCTION_BLOCK_SIZE); block++)
			processNoiseReduction((float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*NOISE_REDUCTION_BLOCK_SIZE), (float32_t *)&FPGA_Audio_Buffer_I_tmp[0] + (block*NOISE_REDUCTION_BLOCK_SIZE));
	}
}

static void doRX_AGC(void)
{
	//AGC
	DoAGC((float32_t *)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
}

static void doRX_SMETER(void)
{
	//Prepare data to calculate s-meter
	for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		if (FPGA_Audio_Buffer_I_tmp[i] > Processor_RX_Audio_Samples_MAX_value) Processor_RX_Audio_Samples_MAX_value = FPGA_Audio_Buffer_I_tmp[i];
		if (FPGA_Audio_Buffer_Q_tmp[i] > Processor_RX_Audio_Samples_MAX_value) Processor_RX_Audio_Samples_MAX_value = FPGA_Audio_Buffer_Q_tmp[i];
		if (FPGA_Audio_Buffer_I_tmp[i] < Processor_RX_Audio_Samples_MIN_value) Processor_RX_Audio_Samples_MIN_value = FPGA_Audio_Buffer_I_tmp[i];
		if (FPGA_Audio_Buffer_Q_tmp[i] < Processor_RX_Audio_Samples_MIN_value) Processor_RX_Audio_Samples_MIN_value = FPGA_Audio_Buffer_Q_tmp[i];
	}
}

static void doRX_COPYCHANNEL(void)
{
	//Double channel I->Q
	dma_memcpy32((uint32_t)&FPGA_Audio_Buffer_Q_tmp[0], (uint32_t)&FPGA_Audio_Buffer_I_tmp[0], FPGA_AUDIO_BUFFER_HALF_SIZE);
}

static void DemodulateFM(void)
{
	float32_t angle, x, y, a, b;
	static float lpf_prev, hpf_prev_a, hpf_prev_b;// used in FM detection and low/high pass processing
	float32_t squelch_buf[FPGA_AUDIO_BUFFER_HALF_SIZE];
	static float32_t i_prev, q_prev;// used in FM detection and low/high pass processing
	static uint8_t fm_sql_count = 0;// used for squelch processing and debouncing tone detection, respectively

	for (uint16_t i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		// first, calculate "x" and "y" for the arctan2, comparing the vectors of present data with previous data
		y = (FPGA_Audio_Buffer_Q_tmp[i] * i_prev) - (FPGA_Audio_Buffer_I_tmp[i] * q_prev);
		x = (FPGA_Audio_Buffer_I_tmp[i] * i_prev) + (FPGA_Audio_Buffer_Q_tmp[i] * q_prev);
		angle = atan2f(y, x);

		// we now have our audio in "angle"
		squelch_buf[i] = angle;	// save audio in "d" buffer for squelch noise filtering/detection - done later

		a = lpf_prev + (FM_RX_LPF_ALPHA * (angle - lpf_prev));	//
		lpf_prev = a;			// save "[n-1]" sample for next iteration

		q_prev = FPGA_Audio_Buffer_Q_tmp[i];// save "previous" value of each channel to allow detection of the change of angle in next go-around
		i_prev = FPGA_Audio_Buffer_I_tmp[i];

		if ((!TRX_squelched) || (!TRX.FM_SQL_threshold)) // high-pass audio only if we are un-squelched (to save processor time)
		{
			if (TRX_getMode() == TRX_MODE_WFM)
			{
				FPGA_Audio_Buffer_I_tmp[i] = (float32_t)(angle / PI * (1 << 14)); //second way
			}
			else
			{
				b = FM_RX_HPF_ALPHA * (hpf_prev_b + a - hpf_prev_a);// do differentiation
				hpf_prev_a = a;		// save "[n-1]" samples for next iteration
				hpf_prev_b = b;
				FPGA_Audio_Buffer_I_tmp[i] = b * 30000.0f;// save demodulated and filtered audio in main audio processing buffer
			}
		}
		else if (TRX_squelched)// were we squelched or tone NOT detected?
			FPGA_Audio_Buffer_I_tmp[i] = 0;// do not filter receive audio - fill buffer with zeroes to mute it
	}

	// *** Squelch Processing ***
	//arm_iir_lattice_f32(&IIR_Squelch_HPF, squelch_buf, squelch_buf, FPGA_AUDIO_BUFFER_HALF_SIZE);	// Do IIR high-pass filter on audio so we may detect squelch noise energy
	fm_sql_avg = ((1.0f - FM_RX_SQL_SMOOTHING) * fm_sql_avg) + (FM_RX_SQL_SMOOTHING * sqrtf(fabsf(squelch_buf[0])));// IIR filter squelch energy magnitude:  We need look at only one representative sample

	// Squelch processing
	// Determine if the (averaged) energy in "ads.fm_sql_avg" is above or below the squelch threshold
	if (fm_sql_count == 0)	// do the squelch threshold calculation much less often than we are called to process this audio
	{
		if (fm_sql_avg > 0.7f)	// limit maximum noise value in averaging to keep it from going out into the weeds under no-signal conditions (higher = noisier)
			fm_sql_avg = 0.7f;
		b = fm_sql_avg * 10.0f;// scale noise amplitude to range of squelch setting
		// Now evaluate noise power with respect to squelch setting
		if (!TRX.FM_SQL_threshold)	 	// is squelch set to zero?
			TRX_squelched = false;		// yes, the we are un-squelched
		else if (TRX_squelched)	 	// are we squelched?
		{
			if (b <= (float)((10 - TRX.FM_SQL_threshold) - FM_SQUELCH_HYSTERESIS))	// yes - is average above threshold plus hysteresis?
				TRX_squelched = false;		//  yes, open the squelch
		}
		else	 	// is the squelch open (e.g. passing audio)?
		{
			if ((10.0f - TRX.FM_SQL_threshold) > FM_SQUELCH_HYSTERESIS)// is setting higher than hysteresis?
			{
				if (b > (float)((10 - TRX.FM_SQL_threshold) + FM_SQUELCH_HYSTERESIS))// yes - is average below threshold minus hysteresis?
					TRX_squelched = true;	// yes, close the squelch
			}
			else	 // setting is lower than hysteresis so we can't use it!
			{
				if (b > (10.0f - (float)TRX.FM_SQL_threshold))// yes - is average below threshold?
					TRX_squelched = true;	// yes, close the squelch
			}
		}
		//
		fm_sql_count++;// bump count that controls how often the squelch threshold is checked
		if (fm_sql_count >= FM_SQUELCH_PROC_DECIMATION)
			fm_sql_count = 0;	// enforce the count limit
	}
}

static void ModulateFM(void)
{
	static uint32_t modulation = TRX_SAMPLERATE;
	static float32_t hpf_prev_a = 0;
	static float32_t hpf_prev_b = 0;
	static float32_t sin_data = 0;
	static uint32_t fm_mod_accum = 0;
	static float32_t modulation_index = 2.0f;
	if (CurrentVFO()->Filter_Width == 5000) modulation_index = 2.0f;
	if (CurrentVFO()->Filter_Width == 6000) modulation_index = 3.0f;
	if (CurrentVFO()->Filter_Width == 7000) modulation_index = 4.0f;
	if (CurrentVFO()->Filter_Width == 8000) modulation_index = 5.0f;
	if (CurrentVFO()->Filter_Width == 9000) modulation_index = 6.0f;
	if (CurrentVFO()->Filter_Width == 9500) modulation_index = 7.0f;
	if (CurrentVFO()->Filter_Width == 10000) modulation_index = 8.0f;
	if (CurrentVFO()->Filter_Width == 15000) modulation_index = 9.0f;
	if (CurrentVFO()->Filter_Width == 0) modulation_index = 10.0f;
	// Do differentiating high-pass filter to provide 6dB/octave pre-emphasis - which also removes any DC component!
	for (int i = 0; i < FPGA_AUDIO_BUFFER_HALF_SIZE; i++)
	{
		float32_t a = FPGA_Audio_Buffer_I_tmp[i];
		hpf_prev_b = FM_TX_HPF_ALPHA * (hpf_prev_b + a - hpf_prev_a);    // do differentiation
		hpf_prev_a = a;     // save "[n-1] samples for next iteration
		fm_mod_accum += hpf_prev_b;   // save differentiated data in audio buffer // change frequency using scaled audio
		fm_mod_accum %= modulation;             // limit range
		sin_data = (fm_mod_accum / (float32_t)modulation)*PI*modulation_index;
		FPGA_Audio_Buffer_I_tmp[i] = selected_rfpower_amplitude * arm_sin_f32(sin_data);
		FPGA_Audio_Buffer_Q_tmp[i] = selected_rfpower_amplitude * arm_cos_f32(sin_data);
	}
}
