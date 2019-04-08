#include "fft.h"
#include "lcd.h"
#include <stdlib.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "functions.h"
#include "audio_processor.h"
#include "wm8731.h"
#include "settings.h"

const static arm_cfft_instance_f32 *S = &arm_cfft_sR_f32_len512;

bool NeedFFTInputBuffer = false; //флаг необходимости заполнения буфера с FPGA
uint32_t FFT_buff_index = 0; //текущий индекс буфера при его наполнении с FPGA
float32_t FFTInput[FFT_SIZE * 2] = { 0 }; //входящий буфер FFT
float32_t FFTOutput[FFT_SIZE] = { 0 }; //результирующий буфер FFT
float32_t FFTOutput_mean[FFT_PRINT_SIZE] = { 0 }; //усредненный буфер FFT (для вывода)
uint16_t wtf_buffer[FFT_WTF_HEIGHT][FFT_PRINT_SIZE] = { 0 }; //буфер водопада
uint16_t maxValueErrors = 0; //количество превышений сигнала в FFT
uint16_t height = 0; //высота столбца в выводе FFT
float32_t maxValueFFT = 0; //максимальное значение амплитуды в результирующей АЧХ

bool FFT_need_fft = true; //необходимо подготовить данные для отображения на экран

void FFT_doFFT(void)
{
	if (!TRX.FFT_Enabled) return;
	if (!FFT_need_fft) return;
	if (NeedFFTInputBuffer) return;
	
	uint32_t maxIndex = 0; // Индекс элемента массива с максимальной амплитудой в результирующей АЧХ
	float32_t maxValue = 0; // Максимальное значение амплитуды в результирующей АЧХ
	float32_t meanValue = 0; // Среднее значение амплитуды в результирующей АЧХ
	float32_t diffValue = 0; // Разница между максимальным значением в FFT и пороге в водопаде
	float32_t hanning_multiplier = 0; //Множитель для вычисления окна Ханнинга к FFT
	
	for (uint16_t i = 0; i < FFT_SIZE; i++) //Hanning window
	{
		hanning_multiplier = 0.5f * (1.0f - arm_cos_f32(2.0f * PI*i / FFT_SIZE*2));
		FFTInput[i * 2] = hanning_multiplier * FFTInput[i * 2];
		FFTInput[i * 2 + 1] = hanning_multiplier * FFTInput[i * 2 + 1];
	}
	arm_cfft_f32(S, FFTInput, 0, 1);
	arm_cmplx_mag_f32(FFTInput, FFTOutput, FFT_SIZE);
	
	//Уменьшаем расчитанный FFT до видимого
	uint8_t fft_compress_rate=FFT_SIZE / FFT_PRINT_SIZE;
	for(uint16_t i=0;i<FFT_PRINT_SIZE;i++)
	{
		float32_t fft_compress_tmp = 0;
		for (uint8_t c = 0; c < fft_compress_rate; c++)
			fft_compress_tmp += FFTOutput[i*fft_compress_rate + c];
		FFTOutput[i] = fft_compress_tmp / fft_compress_rate;
	}
	
	//Автокалибровка уровней FFT
	arm_max_f32(FFTOutput, FFT_PRINT_SIZE, &maxValue, &maxIndex); //ищем максимум в АЧХ
	arm_mean_f32(FFTOutput, FFT_PRINT_SIZE, &meanValue); //ищем среднее в АЧХ
	diffValue=(maxValue-maxValueFFT)/FFT_STEP_COEFF;
	if (maxValueErrors >= FFT_MAX_IN_RED_ZONE && diffValue>0) maxValueFFT+=diffValue;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && diffValue<0 && diffValue<-FFT_STEP_FIX) maxValueFFT+=diffValue;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && maxValueFFT>FFT_STEP_FIX) maxValueFFT-=FFT_STEP_FIX;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && diffValue<0 && diffValue<-FFT_STEP_PRECISION) maxValueFFT+=diffValue;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && maxValueFFT>FFT_STEP_PRECISION) maxValueFFT-=FFT_STEP_PRECISION;
	if((meanValue*4)>maxValueFFT) maxValueFFT=(meanValue*4);
	maxValueErrors = 0;
	if (maxValueFFT < FFT_MIN) maxValueFFT = FFT_MIN;

	//Нормируем АЧХ к единице
	arm_scale_f32(FFTOutput,1.0f/maxValueFFT,FFTOutput,FFT_PRINT_SIZE);
	
	//Усреднение значений для последующего вывода (от резких всплесков)
	arm_add_f32(FFTOutput_mean,FFTOutput,FFTOutput_mean,FFT_PRINT_SIZE);
	arm_scale_f32(FFTOutput_mean,0.5f,FFTOutput_mean,FFT_PRINT_SIZE);

	NeedFFTInputBuffer = true;
	FFT_need_fft = false;
}

void FFT_printFFT(void)
{
	if (LCD_busy) return;
	if (!TRX.FFT_Enabled) return;
	if (FFT_need_fft) return;
	if (LCD_mainMenuOpened) return;
	if (LCD_modeMenuOpened) return;
	if (LCD_bandMenuOpened) return;
	LCD_busy = true;

	uint16_t tmp = 0;
	
	//смещаем водопад вниз c помощью DMA
	for (tmp = FFT_WTF_HEIGHT - 1; tmp > 0; tmp--)
	{
		HAL_DMA_Start(&hdma_memtomem_dma2_stream7, (uint32_t)&wtf_buffer[tmp - 1], (uint32_t)&wtf_buffer[tmp], sizeof(wtf_buffer[tmp - 1])/4);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream7, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}

	uint8_t new_x = 0;
	
	for (uint32_t fft_x = 0; fft_x < FFT_PRINT_SIZE; fft_x++)
	{
		if (fft_x < (FFT_PRINT_SIZE / 2)) new_x = fft_x + (FFT_PRINT_SIZE / 2);
		if (fft_x >= (FFT_PRINT_SIZE / 2)) new_x = fft_x - (FFT_PRINT_SIZE / 2);
		height = FFTOutput_mean[(uint16_t)fft_x] * FFT_MAX_HEIGHT;
		if (height > FFT_MAX_HEIGHT - 1)
		{
			height = FFT_MAX_HEIGHT;
			tmp = COLOR_RED;
			maxValueErrors++;
		}
		else
			tmp = getFFTColor(height);
		wtf_buffer[0][new_x] = tmp;
		if(new_x==(FFT_PRINT_SIZE / 2)) continue;
		LCDDriver_drawFastVLine(new_x, FFT_BOTTOM_OFFSET, -FFT_MAX_HEIGHT, COLOR_BLACK);
		LCDDriver_drawFastVLine(new_x, FFT_BOTTOM_OFFSET, -height, tmp);
	}

	//разделитель и полоса приёма
	LCDDriver_drawFastVLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET, -FFT_MAX_HEIGHT, COLOR_GREEN);
	LCDDriver_drawFastHLine(0, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, FFT_PRINT_SIZE, COLOR_BLACK);
	switch(CurrentVFO()->Mode)
	{
		case TRX_MODE_LSB:
		case TRX_MODE_CW_L:
		case TRX_MODE_DIGI_L:
			LCDDriver_drawFastHLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, -CurrentVFO()->Filter_Width/FFT_HZ_IN_PIXEL, COLOR_GREEN);
			break;
		case TRX_MODE_USB:
		case TRX_MODE_CW_U:
		case TRX_MODE_DIGI_U:
			LCDDriver_drawFastHLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, CurrentVFO()->Filter_Width/FFT_HZ_IN_PIXEL, COLOR_GREEN);
			break;
		case TRX_MODE_NFM:
		case TRX_MODE_AM:
			LCDDriver_drawFastHLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, CurrentVFO()->Filter_Width/FFT_HZ_IN_PIXEL, COLOR_GREEN);
			LCDDriver_drawFastHLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, -CurrentVFO()->Filter_Width/FFT_HZ_IN_PIXEL, COLOR_GREEN);
			break;
		default:
			break;
	}
	
	//выводим на экран водопада с помощью DMA
	LCDDriver_SetCursorAreaPosition(0, FFT_BOTTOM_OFFSET, FFT_PRINT_SIZE-1, FFT_BOTTOM_OFFSET + FFT_WTF_HEIGHT);
	HAL_DMA_Start(&hdma_memtomem_dma2_stream6, (uint32_t)&wtf_buffer, LCD_FSMC_DATA_ADDR, FFT_WTF_HEIGHT*FFT_PRINT_SIZE);
	HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream6, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	
	LCDDriver_drawFastVLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET, FFT_PRINT_SIZE, COLOR_GREEN);
	
	FFT_need_fft = true;
	LCD_busy = false;
}

void FFT_moveWaterfall(int16_t freq_diff)
{
	int16_t new_x = 0;
	freq_diff = freq_diff / FFT_HZ_IN_PIXEL;

	for (uint8_t y = 0; y < FFT_WTF_HEIGHT; y++)
	{
		if (freq_diff > 0) //freq up
		{
			for (int16_t x = 0; x <= FFT_PRINT_SIZE; x++)
			{
				new_x = x + freq_diff;
				if (new_x<0 || new_x>=FFT_PRINT_SIZE)
				{
					wtf_buffer[y][x] = 0;
					continue;
				};
				wtf_buffer[y][x] = wtf_buffer[y][new_x];
			}
		}
		else if (freq_diff < 0) // freq down
		{
			for (int16_t x = FFT_PRINT_SIZE; x >= 0; x--)
			{
				new_x = x + freq_diff;
				if (new_x<=0 || new_x>FFT_PRINT_SIZE)
				{
					wtf_buffer[y][x] = 0;
					continue;
				};
				wtf_buffer[y][x] = wtf_buffer[y][new_x];
			}
		}
	}
}

uint16_t getFFTColor(uint8_t height)
{
	//r g b
	//0 0 0
	//0 0 255
	//255 255 0
	//255 0 0

	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;

	if (height <= FFT_MAX_HEIGHT / 3)
	{
		blue = (height * 255 / (FFT_MAX_HEIGHT / 3));
	}
	else if (height <= 2 * FFT_MAX_HEIGHT / 3)
	{
		green = ((height - FFT_MAX_HEIGHT / 3) * 255 / (FFT_MAX_HEIGHT / 3));
		red = green;
		blue = 255 - green;
	}
	else
	{
		red = ((height - 2 * FFT_MAX_HEIGHT / 3) * 255 / (FFT_MAX_HEIGHT / 3));
		blue = 255 - red;
		green = 255 - red;
	}
	return rgb888torgb565(red, green, blue);
}
