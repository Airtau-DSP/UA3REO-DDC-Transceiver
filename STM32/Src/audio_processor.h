#ifndef AUDIO_PROCESSOR_h
#define AUDIO_PROCESSOR_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"

#define FPGA_AUDIO_BUFFER_SIZE 1024
#define FPGA_AUDIO_BUFFER_HALF_SIZE FPGA_AUDIO_BUFFER_SIZE/2
#define APROCESSOR_BLOCK_SIZE 32

extern void processRxAudio(void);
extern void initAudioProcessor(void);
extern uint32_t AUDIOPROC_samples;
extern uint16_t Processor_AudioBuffer_A[FPGA_AUDIO_BUFFER_SIZE];
extern uint16_t Processor_AudioBuffer_B[FPGA_AUDIO_BUFFER_SIZE];
extern uint8_t Processor_AudioBuffer_ReadyBuffer;
extern bool Processor_NeedBuffer;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream1;
extern uint16_t fpga_index_copy;

#endif
