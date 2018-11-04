/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__

/* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

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
#define FPGA_IN_D2_Pin GPIO_PIN_0
#define FPGA_IN_D2_GPIO_Port GPIOA
#define FPGA_IN_D3_Pin GPIO_PIN_1
#define FPGA_IN_D3_GPIO_Port GPIOA
#define FPGA_IN_D0_Pin GPIO_PIN_2
#define FPGA_IN_D0_GPIO_Port GPIOA
#define FPGA_IN_D1_Pin GPIO_PIN_3
#define FPGA_IN_D1_GPIO_Port GPIOA
#define FPGA_OUT_D0_Pin GPIO_PIN_4
#define FPGA_OUT_D0_GPIO_Port GPIOA
#define FPGA_OUT_D1_Pin GPIO_PIN_5
#define FPGA_OUT_D1_GPIO_Port GPIOA
#define FPGA_OUT_D2_Pin GPIO_PIN_6
#define FPGA_OUT_D2_GPIO_Port GPIOA
#define FPGA_OUT_D3_Pin GPIO_PIN_7
#define FPGA_OUT_D3_GPIO_Port GPIOA
#define PTT_IN_Pin GPIO_PIN_4
#define PTT_IN_GPIO_Port GPIOC
#define PTT_IN_EXTI_IRQn EXTI4_IRQn
#define LED_PEN_Pin GPIO_PIN_5
#define LED_PEN_GPIO_Port GPIOC
#define W26Q16_CS_Pin GPIO_PIN_0
#define W26Q16_CS_GPIO_Port GPIOB
#define LED_BL_Pin GPIO_PIN_1
#define LED_BL_GPIO_Port GPIOB
#define TOUCH_CS_Pin GPIO_PIN_12
#define TOUCH_CS_GPIO_Port GPIOB
#define TOUCH_SCK_Pin GPIO_PIN_13
#define TOUCH_SCK_GPIO_Port GPIOB
#define TOUCH_MISO_Pin GPIO_PIN_14
#define TOUCH_MISO_GPIO_Port GPIOB
#define TOUCH_MOSI_Pin GPIO_PIN_15
#define TOUCH_MOSI_GPIO_Port GPIOB
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

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
/* #define USE_FULL_ASSERT    1U */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
 extern "C" {
#endif
void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
