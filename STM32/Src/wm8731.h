#ifndef WM8731_h
#define WM8731_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include "wire.h"
#include "fpga.h"

#define I2C_ADDRESS_WM8731 0x34

#define CODEC_AUDIO_BUFFER_SIZE FPGA_AUDIO_BUFFER_SIZE*2
extern int32_t CODEC_Audio_Buffer_RX[CODEC_AUDIO_BUFFER_SIZE];
extern int32_t CODEC_Audio_Buffer_TX[CODEC_AUDIO_BUFFER_SIZE];

extern uint8_t WM8731_SampleMode;
extern uint32_t WM8731_DMA_samples;

extern I2S_HandleTypeDef hi2s3;
extern DMA_HandleTypeDef hdma_i2s3_ext_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;

extern bool WM8731_DMA_state;
extern bool WM8731_Buffer_underrun;
void WM8731_SendI2CCommand(uint8_t reg, uint8_t value);
extern void WM8731_Init(void);
extern void start_i2s(void);
void start_i2s_rx_dma(void);
void start_i2s_tx_dma(void);
void start_loopback_dma(void);
void WM8731_TX_mode(void);
void WM8731_RX_mode(void);
void WM8731_TXRX_mode(void);
void I2SEx_Fix(I2S_HandleTypeDef *hi2s);

#endif
