#include "fft.h"
#include "lcd.h"
#include <stdlib.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "functions.h"

uint32_t FFT_buff_index = 0;

float32_t FFTInput[FFT_SIZE * 2] = { 0 };
float32_t FFTOutput[FFT_SIZE];

uint8_t FFT_status;
const static arm_cfft_instance_f32 *S = &arm_cfft_sR_f32_len256;

uint16_t wtf_buffer[FFT_WTF_HEIGHT][FFT_SIZE] = { 0 };

uint32_t maxIndex = 0; // Индекс элемента массива с максимальной амплитудой в результирующей АЧХ
float32_t maxValue = 0; // Максимальное значение амплитуды в результирующей АЧХ
uint16_t height = 0; //высота столбца в выводе FFT
uint16_t maxValueErrors = 0; //количество превышений сигнала в FFT
uint16_t tmp = 0;

bool FFT_need_fft = true; //необходимо полдготовить данные для отображения на экран

void FFT_doFFT(void)
{
	//тестовые семплы
	/*
	for(uint32_t n=0;n<FFT_SIZE; n++)
  {
	// if (n!=0) { SamplesBuf[n] = 0.25 * log(n); } // Логарифм (не определён в нуле).
	// SamplesBuf[n] = exp(0.0-n/16.0); // Экспоненциальный импульс.
	 SamplesBuf[n] = 0.0 + sin((n*2.0*3.14159)/512); // Синус.
	//SamplesBuf[n] = (1.0 - (n/512.0))*sin((n*2.0*3.14159)/64); // Затухающий синус.
	// SamplesBuf[n] = 0.75*sin((n*2.0*3.14159)/256) + sin((n*2.0*3.14159)/85.3); // Синус, 2 + 6 гармоники.
	// SamplesBuf[n] = sin((n*2.0*3.14159)/512) + 0.25*sin((n*2.0*3.14159)/32); // Синус + dx-ВЧ гармоника.
	// SamplesBuf[n] = ((n%64)/64.0); // Пилообразный сигнал
		// if (n<16) SamplesBuf[n] = 1.0; else SamplesBuf[n] = 0; // Единичный прямоугольный импульс
  }
	for(uint32_t n=0;n<FFT_SIZE;n++)
  {
	FFTInput[n*2] = SamplesBuf[n];
	FFTInput[n*2+1] = 0.0; // Обнуляем комплексную часть входных данных.
  }
	*/

	/* Process the data through the CFFT/CIFFT module */
	arm_cfft_f32(S, FFTInput, 0, 1);
	/* Process the data through the Complex Magnitude Module for calculating the magnitude at each bin */
	arm_cmplx_mag_f32(FFTInput, FFTOutput, FFT_SIZE);
	/* Calculates maxValue and returns corresponding BIN value */
	//arm_max_f32(FFTOutput, FFT_SIZE, &maxValue, &maxIndex); //ищем максимум
	FFT_need_fft = false;
}

void FFT_printFFT(void)
{
	if (FFT_need_fft) return;
	if (maxValueErrors > 100 || maxValueErrors == 0) arm_max_f32(FFTOutput, FFT_SIZE, &maxValue, &maxIndex); //ищем максимум
	if (maxValue < FFT_CONTRAST) maxValue = FFT_CONTRAST; //минимальный порог
	maxValueErrors = 0;
	// Нормируем АЧХ к единице
	for (uint32_t n = 0; n < FFT_SIZE; n++) FFTOutput[n] = FFTOutput[n] / maxValue;

	ILI9341_Fill_RectWH(0, FFT_BOTTOM_OFFSET - FFT_MAX_HEIGHT - 1, FFT_SIZE, FFT_MAX_HEIGHT, COLOR_BLACK);

	ILI9341_drawFastVLine(FFT_SIZE / 2, FFT_BOTTOM_OFFSET - FFT_MAX_HEIGHT, (240 - FFT_BOTTOM_OFFSET) + FFT_MAX_HEIGHT, COLOR_GREEN);

	for (tmp = FFT_WTF_HEIGHT - 1; tmp > 0; tmp--) //смещаем водопад вниз
		memcpy(&wtf_buffer[tmp], &wtf_buffer[tmp - 1], sizeof(wtf_buffer[tmp - 1]));

	uint8_t new_x = 0;
	for (uint32_t fft_x = 0; fft_x < FFT_SIZE; fft_x++)
	{
		if (fft_x < (FFT_SIZE / 2)) new_x = fft_x + (FFT_SIZE / 2);
		if (fft_x >= (FFT_SIZE / 2)) new_x = fft_x - (FFT_SIZE / 2);
		if ((new_x + 1) == FFT_SIZE / 2) continue;
		height = FFTOutput[(uint16_t)fft_x] * FFT_MAX_HEIGHT;
		if (height > FFT_MAX_HEIGHT)
		{
			height = FFT_MAX_HEIGHT;
			tmp = COLOR_RED;
			maxValueErrors++;
		}
		else
			tmp = getFFTColor(height);
		wtf_buffer[0][new_x] = tmp;
		ILI9341_drawFastVLine(new_x + 1, FFT_BOTTOM_OFFSET, -height, tmp);
	}

	for (uint8_t y = 0; y < FFT_WTF_HEIGHT; y++)
	{
		for (uint16_t x = 0; x < FFT_SIZE; x++)
		{
			if ((x + 1) == FFT_SIZE / 2) continue;
			ILI9341_DrawPixel(x + 1, FFT_BOTTOM_OFFSET + y, wtf_buffer[y][x]);
		}
	}

	if (maxValueErrors > 10) maxValue += 1000;
	if (maxValueErrors > 30) maxValue += 3000;
	if (maxValueErrors > 50) maxValue += 5000;
	//logToUART1_float32(maxValue);
	FFT_need_fft = true;
}

uint16_t getFFTColor(uint8_t height)
{
	//b r
	//0 0
	//255 0
	//255 255
	//0 255
	uint8_t blue = 0;
	uint8_t red = 0;
	if (height <= FFT_MAX_HEIGHT / 3) blue = (height * 255 / (FFT_MAX_HEIGHT / 3));
	if (height > FFT_MAX_HEIGHT / 3 && height <= 2 * FFT_MAX_HEIGHT / 3)
	{
		blue = 255;
		red = ((height - FFT_MAX_HEIGHT / 3) * 255 / (FFT_MAX_HEIGHT / 3));
	}
	if (height > 2 * FFT_MAX_HEIGHT / 3)
	{
		blue = 255 - ((height - 2 * FFT_MAX_HEIGHT / 3) * 255 / (FFT_MAX_HEIGHT / 3));
		red = 255;
	}
	return rgb888torgb565(red, 0, blue);
}
