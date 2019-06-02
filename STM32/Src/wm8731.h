#ifndef WM8731_h
#define WM8731_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include "wire.h"
#include "fpga.h"
#include "audio_processor.h"

#define I2C_ADDRESS_WM8731 0x34

#define CODEC_AUDIO_BUFFER_SIZE FPGA_AUDIO_BUFFER_SIZE*2
extern int32_t CODEC_Audio_Buffer_RX[CODEC_AUDIO_BUFFER_SIZE];
extern int32_t CODEC_Audio_Buffer_TX[CODEC_AUDIO_BUFFER_SIZE];
volatile extern bool WM8731_DMA_state;
volatile extern bool WM8731_Buffer_underrun;
volatile extern uint32_t WM8731_DMA_samples;

extern I2S_HandleTypeDef hi2s3;
extern DMA_HandleTypeDef hdma_i2s3_ext_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;

extern void WM8731_Init(void);
extern void WM8731_start_i2s_and_dma(void);
extern void WM8731_Beep(void);
extern void WM8731_TX_mode(void);
extern void WM8731_RX_mode(void);
extern void WM8731_TXRX_mode(void);

#endif
