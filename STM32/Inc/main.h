/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern I2S_HandleTypeDef hi2s3;
extern DMA_HandleTypeDef hdma_i2s3_ext_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;

extern IWDG_HandleTypeDef hiwdg;

extern RTC_HandleTypeDef hrtc;

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim6;

extern UART_HandleTypeDef huart1;

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream1;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream7;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream6;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream5;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream2;
extern SRAM_HandleTypeDef hsram1;

extern volatile uint32_t cpu_sleep_counter;
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ENC_DT_Pin GPIO_PIN_2
#define ENC_DT_GPIO_Port GPIOE
#define ENC_DT_EXTI_IRQn EXTI2_IRQn
#define K1_Pin GPIO_PIN_3
#define K1_GPIO_Port GPIOE
#define K0_Pin GPIO_PIN_4
#define K0_GPIO_Port GPIOE
#define ENC_CLK_Pin GPIO_PIN_5
#define ENC_CLK_GPIO_Port GPIOE
#define FPGA_CLK_Pin GPIO_PIN_0
#define FPGA_CLK_GPIO_Port GPIOC
#define FPGA_SYNC_Pin GPIO_PIN_1
#define FPGA_SYNC_GPIO_Port GPIOC
#define SWR_FORW_Pin GPIO_PIN_2
#define SWR_FORW_GPIO_Port GPIOC
#define SWR_BACKW_Pin GPIO_PIN_3
#define SWR_BACKW_GPIO_Port GPIOC
#define FPGA_BUS_D0_Pin GPIO_PIN_0
#define FPGA_BUS_D0_GPIO_Port GPIOA
#define FPGA_BUS_D1_Pin GPIO_PIN_1
#define FPGA_BUS_D1_GPIO_Port GPIOA
#define FPGA_BUS_D2_Pin GPIO_PIN_2
#define FPGA_BUS_D2_GPIO_Port GPIOA
#define FPGA_BUS_D3_Pin GPIO_PIN_3
#define FPGA_BUS_D3_GPIO_Port GPIOA
#define FPGA_BUS_D4_Pin GPIO_PIN_4
#define FPGA_BUS_D4_GPIO_Port GPIOA
#define FPGA_BUS_D5_Pin GPIO_PIN_5
#define FPGA_BUS_D5_GPIO_Port GPIOA
#define FPGA_BUS_D6_Pin GPIO_PIN_6
#define FPGA_BUS_D6_GPIO_Port GPIOA
#define FPGA_BUS_D7_Pin GPIO_PIN_7
#define FPGA_BUS_D7_GPIO_Port GPIOA
#define PTT_IN_Pin GPIO_PIN_4
#define PTT_IN_GPIO_Port GPIOC
#define PTT_IN_EXTI_IRQn EXTI4_IRQn
#define LED_PEN_Pin GPIO_PIN_5
#define LED_PEN_GPIO_Port GPIOC
#define LED_PEN_EXTI_IRQn EXTI9_5_IRQn
#define W26Q16_CS_Pin GPIO_PIN_0
#define W26Q16_CS_GPIO_Port GPIOB
#define LCD_BACKLIGT_Pin GPIO_PIN_1
#define LCD_BACKLIGT_GPIO_Port GPIOB
#define AUDIO_48K_CLOCK_Pin GPIO_PIN_10
#define AUDIO_48K_CLOCK_GPIO_Port GPIOB
#define AUDIO_48K_CLOCK_EXTI_IRQn EXTI15_10_IRQn
#define TOUCH_CS_Pin GPIO_PIN_12
#define TOUCH_CS_GPIO_Port GPIOB
#define TOUCH_SCK_Pin GPIO_PIN_13
#define TOUCH_SCK_GPIO_Port GPIOB
#define TOUCH_MISO_Pin GPIO_PIN_14
#define TOUCH_MISO_GPIO_Port GPIOB
#define TOUCH_MOSI_Pin GPIO_PIN_15
#define TOUCH_MOSI_GPIO_Port GPIOB
#define ESP_1_TX_Pin GPIO_PIN_6
#define ESP_1_TX_GPIO_Port GPIOC
#define ESP_1_RX_Pin GPIO_PIN_7
#define ESP_1_RX_GPIO_Port GPIOC
#define AUDIO_I2S_CLOCK_Pin GPIO_PIN_9
#define AUDIO_I2S_CLOCK_GPIO_Port GPIOC
#define UART_DEBUG_TX_Pin GPIO_PIN_9
#define UART_DEBUG_TX_GPIO_Port GPIOA
#define UART_DEBUG_RX_Pin GPIO_PIN_10
#define UART_DEBUG_RX_GPIO_Port GPIOA
#define WM8731_WS_LRC_Pin GPIO_PIN_15
#define WM8731_WS_LRC_GPIO_Port GPIOA
#define WM8731_BCLK_Pin GPIO_PIN_10
#define WM8731_BCLK_GPIO_Port GPIOC
#define WM8731_ADC_SD_Pin GPIO_PIN_11
#define WM8731_ADC_SD_GPIO_Port GPIOC
#define WM8731_DAC_SD_Pin GPIO_PIN_12
#define WM8731_DAC_SD_GPIO_Port GPIOC
#define WM8731_SCK_Pin GPIO_PIN_3
#define WM8731_SCK_GPIO_Port GPIOD
#define WM8731_SDA_Pin GPIO_PIN_6
#define WM8731_SDA_GPIO_Port GPIOD
#define W25Q16_SCK_Pin GPIO_PIN_3
#define W25Q16_SCK_GPIO_Port GPIOB
#define W25Q16_MISO_Pin GPIO_PIN_4
#define W25Q16_MISO_GPIO_Port GPIOB
#define W25Q16_MOSI_Pin GPIO_PIN_5
#define W25Q16_MOSI_GPIO_Port GPIOB
#define RFUNIT_RCLK_Pin GPIO_PIN_6
#define RFUNIT_RCLK_GPIO_Port GPIOB
#define RFUNIT_CLK_Pin GPIO_PIN_7
#define RFUNIT_CLK_GPIO_Port GPIOB
#define RFUNIT_DATA_Pin GPIO_PIN_8
#define RFUNIT_DATA_GPIO_Port GPIOB
#define KEY_IN_DASH_Pin GPIO_PIN_0
#define KEY_IN_DASH_GPIO_Port GPIOE
#define KEY_IN_DOT_Pin GPIO_PIN_1
#define KEY_IN_DOT_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
