#include "fft.h"
#include "lcd.h"
#include <stdlib.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "functions.h"

uint32_t FFT_buff_index = 0;
float32_t FFTInput_A[FFT_SIZE * 2] = { 0 };
float32_t FFTInput_B[FFT_SIZE * 2] = { 0 };
bool FFTInputBufferInProgress = false; //false - A, true - B
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
	if(!FFT_need_fft) return;
	if(FFTInputBufferInProgress) //B in progress
	{
		for (int i = 0; i < FFT_SIZE; i++) //Hanning window
		{
			double multiplier = (float32_t)0.5 * ((float32_t)1 - arm_cos_f32(2*PI*i/FFT_SIZE));
			FFTInput_A[i*2] = multiplier * FFTInput_A[i*2];
			FFTInput_A[i*2+1] = multiplier * FFTInput_A[i*2+1];
		}
		arm_cfft_f32(S, FFTInput_A, 0, 1);
		arm_cmplx_mag_squared_f32(FFTInput_A, FFTOutput, FFT_SIZE);
	}
	else //A in progress
	{
		for (int i = 0; i < FFT_SIZE; i++) //Hanning window
		{
			double multiplier = (float32_t)0.5 * ((float32_t)1 - arm_cos_f32(2*PI*i/FFT_SIZE));
			FFTInput_B[i*2] = multiplier * FFTInput_B[i*2];
			FFTInput_B[i*2+1] = multiplier * FFTInput_B[i*2+1];
		}
		arm_cfft_f32(S, FFTInput_B, 0, 1);
		arm_cmplx_mag_squared_f32(FFTInput_B, FFTOutput, FFT_SIZE);
	}
	FFT_need_fft = false;
}

void FFT_printFFT(void)
{
	if(LCD_busy) return;
	if (FFT_need_fft) return;
	if (LCD_mainMenuOpened) return;
	LCD_busy=true;
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
	LCD_busy=false;
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
