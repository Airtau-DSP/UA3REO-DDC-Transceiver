/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#include "functions.h"
#include "encoder.h"
#include "fpga.h"
#include "lcd.h"
#include "wm8731.h"
#include "audio_processor.h"
#include "agc.h"
#include "fft.h"
#include "settings.h"
#include "fpga.h"
#include "profiler.h"
#include "usbd_debug_if.h"
#include "usbd_cat_if.h"
#include "usbd_audio_if.h"
#include "usbd_ua3reo.h"

uint32_t ms50_counter = 0;
uint32_t tim5_counter = 0;
uint32_t eeprom_save_delay = 0;
uint8_t dc_correction_state = 0;

extern I2S_HandleTypeDef hi2s3;
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream1;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream7;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream6;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream5;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream2;
extern DMA_HandleTypeDef hdma_i2s3_ext_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim6;
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
/* USER CODE BEGIN EV */
extern uint32_t cpu_sleep_counter;
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */ 
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line2 interrupt.
  */
void EXTI2_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI2_IRQn 0 */

  /* USER CODE END EXTI2_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
  /* USER CODE BEGIN EXTI2_IRQn 1 */
	TRX_Time_InActive=0;
	if (TRX_inited) ENCODER_checkRotate();
  /* USER CODE END EXTI2_IRQn 1 */
}

/**
  * @brief This function handles EXTI line4 interrupt.
  */
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */

  /* USER CODE END EXTI4_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
  /* USER CODE BEGIN EXTI4_IRQn 1 */
	TRX_Time_InActive=0;
	if (TRX_inited && TRX_getMode() != TRX_MODE_NO_TX) TRX_ptt_change();
  /* USER CODE END EXTI4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream0 global interrupt.
  */
void DMA1_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream0_IRQn 0 */

  /* USER CODE END DMA1_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_i2s3_ext_rx);
  /* USER CODE BEGIN DMA1_Stream0_IRQn 1 */

  /* USER CODE END DMA1_Stream0_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream5 global interrupt.
  */
void DMA1_Stream5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */

  /* USER CODE END DMA1_Stream5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi3_tx);
  /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */

  /* USER CODE END DMA1_Stream5_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[9:5] interrupts.
  */
void EXTI9_5_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */

  /* USER CODE END EXTI9_5_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
  /* USER CODE BEGIN EXTI9_5_IRQn 1 */
	TRX_Time_InActive=0;
	LCD_checkTouchPad();
  /* USER CODE END EXTI9_5_IRQn 1 */
}

/**
  * @brief This function handles TIM4 global interrupt.
  */
void TIM4_IRQHandler(void)
{
  /* USER CODE BEGIN TIM4_IRQn 0 */

  /* USER CODE END TIM4_IRQn 0 */
  HAL_TIM_IRQHandler(&htim4);
  /* USER CODE BEGIN TIM4_IRQn 1 */
	DEBUG_Transmit_FIFO_Events();
	ua3reo_dev_cat_parseCommand();
	if (FFT_need_fft) FFT_doFFT();
  /* USER CODE END TIM4_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */
	if (!FPGA_busy) FPGA_fpgadata_iqclock();
	if (!FPGA_busy) FPGA_fpgadata_stuffclock();
  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
  * @brief This function handles TIM5 global interrupt.
  */
void TIM5_IRQHandler(void)
{
  /* USER CODE BEGIN TIM5_IRQn 0 */

  /* USER CODE END TIM5_IRQn 0 */
  HAL_TIM_IRQHandler(&htim5);
  /* USER CODE BEGIN TIM5_IRQn 1 */
	tim5_counter++;
	if (TRX_on_TX())
	{
		if (TRX_getMode() != TRX_MODE_NO_TX)
			processTxAudio();
	}
	else
	{
		processRxAudio();
	}
  /* USER CODE END TIM5_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts.
  */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */
	ms50_counter++;
	
	const float32_t dc_correction_step_fast=1.0f;
	const float32_t dc_correction_step_slow=0.1f;
	const float32_t dc_correction_step_precision=0.001f;
	float32_t dc_correction_step = 0;
	
	if(dc_correction_state==2) dc_correction_step=dc_correction_step_precision;
	else if(dc_correction_state==1) dc_correction_step=dc_correction_step_slow;
	else if(dc_correction_state==0) dc_correction_step=dc_correction_step_fast;

	if(FPGA_MIN_I_Value<0 && FPGA_MAX_I_Value<0)
		FPGA_DC_Offset+=dc_correction_step;
	else if(FPGA_MIN_I_Value>0 && FPGA_MAX_I_Value>0)
		FPGA_DC_Offset-=dc_correction_step;
	else if(-FPGA_MIN_I_Value>FPGA_MAX_I_Value)
	{
		if((FPGA_MIN_I_Value+FPGA_MAX_I_Value)>-dc_correction_step) dc_correction_step=0;
		if((dc_correction_state==2) && (FPGA_MIN_I_Value+FPGA_MAX_I_Value)>-dc_correction_step_fast) dc_correction_step=0;
		FPGA_DC_Offset+=dc_correction_step;
		if(dc_correction_state!=2) dc_correction_state=1;
	}
	else if(-FPGA_MIN_I_Value<FPGA_MAX_I_Value)
	{
		if((FPGA_MIN_I_Value+FPGA_MAX_I_Value)<dc_correction_step) dc_correction_step=0;
		if((dc_correction_state==2) && (FPGA_MIN_I_Value+FPGA_MAX_I_Value)<dc_correction_step_fast) dc_correction_step=0;
		FPGA_DC_Offset-=dc_correction_step;
		if(dc_correction_state!=2)
		{
			dc_correction_state=1;
			if((FPGA_MIN_I_Value+FPGA_MAX_I_Value)<dc_correction_step_slow)
				dc_correction_state=2;
		}
	}
	//if(dc_correction_step>0) sendToDebug_float32(FPGA_MIN_I_Value+FPGA_MAX_I_Value,false);
	
	FPGA_MIN_I_Value=32700;
	FPGA_MAX_I_Value=-32700;
	
	if(TRX_Key_Timeout_est>0)
	{
		TRX_Key_Timeout_est-=50;
		if(TRX_Key_Timeout_est==0)
		{
			LCD_UpdateQuery.StatusInfoGUI = true;
			FPGA_NeedSendParams = true;
			TRX_Restart_Mode();
		}
	}
	
	if (NeedSaveSettings)
		FPGA_NeedSendParams = true;
	
	if ((ms50_counter % 2) == 0) // every 100ms
	{
		//S-Meter Calculate
		float32_t Audio_Vpp_value=(Processor_RX_Audio_Samples_MAX_value/(float32_t)TRX.RF_Gain)-(Processor_RX_Audio_Samples_MIN_value/(float32_t)TRX.RF_Gain); //получаем разницу между максимальным и минимальным значением в аудио-семплах
		for(int i=0;i<(FPGA_BUS_BITS-ADC_BITS);i++) Audio_Vpp_value=Audio_Vpp_value/2; //приводим разрядность аудио к разрядности АЦП
		float32_t ADC_Vpp_Value=Audio_Vpp_value*ADC_VREF/(pow(2,ADC_BITS)-1); //получаем значение пик-пик напряжения на входе АЦП в вольтах
		float32_t ADC_Vrms_Value=ADC_Vpp_Value*0.3535f; // Получаем действующее (RMS) напряжение на аходе АЦП
		float32_t ADC_RF_IN_Value=(ADC_Vrms_Value/ADC_RF_TRANS_RATIO)*ADC_RF_INPUT_VALUE_CALIBRATION; //Получаем напряжение на антенном входе с учётом трансформатора и калибровки
		if(ADC_RF_IN_Value<0.0000001f) ADC_RF_IN_Value=0.0000001f;
		TRX_RX_dBm=10*log10f_fast((ADC_RF_IN_Value*ADC_RF_IN_Value)/(ADC_RESISTANCE*0.001)) ; //получаем значение мощности в dBm для сопротивления входа АЦП
		Processor_RX_Audio_Samples_MAX_value=0;
		Processor_RX_Audio_Samples_MIN_value=0;
		//
	}
	
	if (ms50_counter == 20) // every 1 sec
	{
		ms50_counter = 0;
		
		//Save Debug variables
		uint32_t dbg_FPGA_samples=FPGA_samples;
		uint32_t dbg_WM8731_DMA_samples=WM8731_DMA_samples/2;
		uint32_t dbg_AUDIOPROC_TXA_samples=AUDIOPROC_TXA_samples;
		uint32_t dbg_AUDIOPROC_TXB_samples=AUDIOPROC_TXB_samples;
		float32_t dbg_cpu_sleep_counter=cpu_sleep_counter/1000.0f;
		uint32_t dbg_tim5_counter=tim5_counter;
		float32_t dbg_FPGA_DC_Offset=FPGA_DC_Offset;
		float32_t dbg_ALC_need_gain=ALC_need_gain;
		float32_t dbg_ALC_need_gain_new=ALC_need_gain_new;
		float32_t dbg_Processor_TX_MAX_amplitude=Processor_TX_MAX_amplitude;
		float32_t dbg_FPGA_Audio_Buffer_I_tmp=FPGA_Audio_Buffer_I_tmp[0];
		float32_t dbg_FPGA_Audio_Buffer_Q_tmp=FPGA_Audio_Buffer_Q_tmp[0];
		uint32_t dbg_RX_USB_AUDIO_SAMPLES=RX_USB_AUDIO_SAMPLES;
		uint32_t dbg_TX_USB_AUDIO_SAMPLES=TX_USB_AUDIO_SAMPLES;
		//sendToDebug_str("FPGA Samples: "); sendToDebug_uint32(dbg_FPGA_samples,false); //~48000
		//sendToDebug_str("Audio DMA samples: "); sendToDebug_uint32(dbg_WM8731_DMA_samples,false); //~48000
		//sendToDebug_str("Audioproc cycles A: "); sendToDebug_uint32(dbg_AUDIOPROC_TXA_samples,false); //~187
		//sendToDebug_str("Audioproc cycles B: "); sendToDebug_uint32(dbg_AUDIOPROC_TXB_samples,false); //~187
		//sendToDebug_str("CPU Sleep counter: "); sendToDebug_float32(dbg_cpu_sleep_counter,false);  
		//sendToDebug_str("Audioproc timer counter: "); sendToDebug_uint32(dbg_tim5_counter,false); 
		//sendToDebug_str("DC Offset: "); sendToDebug_float32(dbg_FPGA_DC_Offset,false); 
		//sendToDebug_str("TX Autogain: "); sendToDebug_float32(dbg_ALC_need_gain,false);
		//sendToDebug_str("TX Autogain Target: "); sendToDebug_float32(dbg_ALC_need_gain_new,false);
		//sendToDebug_str("Processor TX MAX amplitude: "); sendToDebug_float32(dbg_Processor_TX_MAX_amplitude,false);
		//sendToDebug_str("First byte of I: "); sendToDebug_float32(dbg_FPGA_Audio_Buffer_I_tmp,false); //first byte of I
		//sendToDebug_str("First byte of Q: "); sendToDebug_float32(dbg_FPGA_Audio_Buffer_Q_tmp,false); //first byte of Q
		//sendToDebug_str("USB Audio RX samples: "); sendToDebug_uint32(dbg_RX_USB_AUDIO_SAMPLES,false); //~48000
		//sendToDebug_str("USB Audio TX samples: "); sendToDebug_uint32(dbg_TX_USB_AUDIO_SAMPLES,false); //~48000
		//sendToDebug_newline();
		//PrintProfilerResult();
		
		tim5_counter = 0;
		FPGA_samples = 0;
		AUDIOPROC_samples = 0;
		AUDIOPROC_TXA_samples = 0;
		AUDIOPROC_TXB_samples = 0;
		WM8731_DMA_samples = 0;
		RX_USB_AUDIO_SAMPLES = 0;
		TX_USB_AUDIO_SAMPLES = 0;
		cpu_sleep_counter = 0;
		TRX_Time_InActive++;
		WM8731_Buffer_underrun = false;
		FPGA_Buffer_underrun = false;
		RX_USB_AUDIO_underrun = false;
		FPGA_NeedSendParams = true;
		FPGA_NeedGetParams = true;
		if (NeedSaveSettings)
		{
			if(eeprom_save_delay<30) //Запись в EEPROM не чаще, чем раз в 30 секунд (против износа)
			{
				eeprom_save_delay++;
			}
			else
			{
				SaveSettings();
				eeprom_save_delay=0;
			}
		}
	}
	
	if (TRX_ptt_hard == HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin)) TRX_ptt_change();
	if (TRX_ptt_cat != TRX_old_ptt_cat) TRX_ptt_change();
	if (TRX_key_serial != TRX_old_key_serial) TRX_key_change();
	if (TRX_key_hard == HAL_GPIO_ReadPin(KEY_IN_GPIO_Port, KEY_IN_Pin)) TRX_key_change();
	
	LCD_doEvents();
	FFT_printFFT();
  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream0 global interrupt.
  */
void DMA2_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream0_IRQn 0 */

	//DMA, занимающийся пересылкой данных в буфер кодека при приёме
	
  /* USER CODE END DMA2_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_memtomem_dma2_stream0);
  /* USER CODE BEGIN DMA2_Stream0_IRQn 1 */

  /* USER CODE END DMA2_Stream0_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream1 global interrupt.
  */
void DMA2_Stream1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream1_IRQn 0 */
	
	//DMA, занимающийся пересылкой данных в буфер кодека при приёме

  /* USER CODE END DMA2_Stream1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_memtomem_dma2_stream1);
  /* USER CODE BEGIN DMA2_Stream1_IRQn 1 */

  /* USER CODE END DMA2_Stream1_IRQn 1 */
}

/**
  * @brief This function handles USB On The Go FS global interrupt.
  */
void OTG_FS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_FS_IRQn 0 */

  /* USER CODE END OTG_FS_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_FS_IRQn 1 */

  /* USER CODE END OTG_FS_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream6 global interrupt.
  */
void DMA2_Stream6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream6_IRQn 0 */
	
	//DMA, занимающийся отрисовкой водопада
	
  /* USER CODE END DMA2_Stream6_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_memtomem_dma2_stream6);
  /* USER CODE BEGIN DMA2_Stream6_IRQn 1 */
	LCDDriver_drawFastVLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET, FFT_PRINT_SIZE, COLOR_GREEN);
	FFT_need_fft = true;
	LCD_busy = false;
  /* USER CODE END DMA2_Stream6_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
