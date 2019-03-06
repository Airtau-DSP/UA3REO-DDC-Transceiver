#include "fft.h"
#include "lcd.h"
#include <stdlib.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "functions.h"
#include "audio_processor.h"
#include "wm8731.h"

uint32_t FFT_buff_index = 0;
float32_t FFTInput_A[FFT_SIZE * 2] = { 0 };
float32_t FFTInput_B[FFT_SIZE * 2] = { 0 };
bool FFTInputBufferInProgress = false; //false - A, true - B
float32_t FFTOutput[FFT_SIZE];

uint8_t FFT_status;
const static arm_cfft_instance_f32 *S = &arm_cfft_sR_f32_len512;

uint16_t wtf_buffer[FFT_WTF_HEIGHT][FFT_SIZE] = { 0 };

uint32_t maxIndex = 0; // Индекс элемента массива с максимальной амплитудой в результирующей АЧХ
float32_t maxValue = 0; // Максимальное значение амплитуды в результирующей АЧХ
uint16_t height = 0; //высота столбца в выводе FFT
uint16_t maxValueErrors = 0; //количество превышений сигнала в FFT
uint16_t tmp = 0;
uint8_t fft_compress_rate = FFT_SIZE / FFT_PRINT_SIZE;
float32_t fft_compress_tmp = 0;

bool FFT_need_fft = true; //необходимо полдготовить данные для отображения на экран

void FFT_doFFT(void)
{
	if (!FFT_need_fft) return;
	if (FFTInputBufferInProgress) //B in progress
	{
		for (int i = 0; i < FFT_SIZE; i++) //Hanning window
		{
			double multiplier = (float32_t)0.5 * ((float32_t)1 - arm_cos_f32(2 * PI*i / FFT_SIZE));
			FFTInput_A[i * 2] = multiplier * FFTInput_A[i * 2];
			FFTInput_A[i * 2 + 1] = multiplier * FFTInput_A[i * 2 + 1];
		}
		arm_cfft_f32(S, FFTInput_A, 0, 1);
		//arm_cmplx_mag_f32(FFTInput_A, FFTOutput, FFT_SIZE);
		arm_cmplx_mag_squared_f32(FFTInput_A, FFTOutput, FFT_SIZE);
	}
	else //A in progress
	{
		for (int i = 0; i < FFT_SIZE; i++) //Hanning window
		{
			double multiplier = (float32_t)0.5 * ((float32_t)1 - arm_cos_f32(2 * PI*i / FFT_SIZE));
			FFTInput_B[i * 2] = multiplier * FFTInput_B[i * 2];
			FFTInput_B[i * 2 + 1] = multiplier * FFTInput_B[i * 2 + 1];
		}
		arm_cfft_f32(S, FFTInput_B, 0, 1);
		//arm_cmplx_mag_f32(FFTInput_B, FFTOutput, FFT_SIZE);
		arm_cmplx_mag_squared_f32(FFTInput_B, FFTOutput, FFT_SIZE);
	}
	//Min and Max on FFT print
	if (maxValueErrors > 10 || maxValueErrors == 0)
	{
		arm_max_f32(FFTOutput, FFT_SIZE, &maxValue, &maxIndex); //ищем максимум
		//if (maxValue > Processor_AVG_amplitude*FFT_MAX) maxValue = Processor_AVG_amplitude * FFT_MAX;
	}
	maxValueErrors = 0;
	if(maxValue<FFT_MIN) maxValue=FFT_MIN;
	// Нормируем АЧХ к единице
	for (uint16_t n = 0; n < FFT_SIZE; n++)
	{
		FFTOutput[n] = FFTOutput[n] / maxValue;
		if (FFTOutput[n] > 1) FFTOutput[n] = 1;
	}
	//Compress FFT_SIZE to FFT_PRINT_SIZE
	for (uint16_t n = 0; n < FFT_PRINT_SIZE; n++)
	{
		fft_compress_tmp = 0;
		for (uint8_t c = 0; c < fft_compress_rate; c++)
			fft_compress_tmp += FFTOutput[n*fft_compress_rate + c];
		FFTOutput[n] = fft_compress_tmp / fft_compress_rate;
		if (FFTOutput[n] > 1) FFTOutput[n] = 1;
	}
	//
	FFT_need_fft = false;
}

void FFT_printFFT(void)
{
	if (LCD_busy) return;
	if (TRX_ptt || TRX_tune || TRX_getMode() == TRX_MODE_LOOPBACK) return;
	if (FFT_need_fft) return;
	if (LCD_mainMenuOpened) return;
	LCD_busy = true;
	
	ILI9341_drawFastVLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET - FFT_MAX_HEIGHT, (240 - FFT_BOTTOM_OFFSET) + FFT_MAX_HEIGHT, COLOR_GREEN);

	for (tmp = FFT_WTF_HEIGHT - 1; tmp > 0; tmp--) //смещаем водопад вниз
		memcpy(&wtf_buffer[tmp], &wtf_buffer[tmp - 1], sizeof(wtf_buffer[tmp - 1]));

	uint8_t new_x = 0;
	for (uint32_t fft_x = 0; fft_x < FFT_PRINT_SIZE; fft_x++)
	{
		if (fft_x < (FFT_PRINT_SIZE / 2)) new_x = fft_x + (FFT_PRINT_SIZE / 2);
		if (fft_x >= (FFT_PRINT_SIZE / 2)) new_x = fft_x - (FFT_PRINT_SIZE / 2);
		if ((new_x + 1) == FFT_PRINT_SIZE / 2) continue;
		height = FFTOutput[(uint16_t)fft_x] * FFT_MAX_HEIGHT;
		if (height > FFT_MAX_HEIGHT-1)
		{
			height = FFT_MAX_HEIGHT;
			tmp = COLOR_RED;
			maxValueErrors++;
		}
		else
			tmp = getFFTColor(height);

		wtf_buffer[0][new_x] = tmp;
		ILI9341_drawFastVLine(new_x + 1, FFT_BOTTOM_OFFSET, -FFT_MAX_HEIGHT - 1, COLOR_BLACK);
		ILI9341_drawFastVLine(new_x + 1, FFT_BOTTOM_OFFSET, -height, tmp);
	}

	for (uint8_t y = 0; y < FFT_WTF_HEIGHT; y++)
	{
		for (uint16_t x = 0; x < FFT_PRINT_SIZE; x++)
		{
			if ((x + 1) == FFT_PRINT_SIZE / 2) continue;
			ILI9341_DrawPixel(x + 1, FFT_BOTTOM_OFFSET + y, wtf_buffer[y][x]);
		}
	}
	
	FFT_need_fft = true;
	LCD_busy = false;
}

void FFT_moveWaterfall(int16_t freq_diff)
{
	int16_t new_x=0;
	freq_diff=freq_diff/FFT_HZ_IN_PIXEL;
	
	for (uint8_t y = 0; y < FFT_WTF_HEIGHT; y++)
	{
		if(freq_diff>0)
		{
			for (int16_t x = 0; x <= FFT_PRINT_SIZE; x++)
			{
				new_x=x+freq_diff;
				if(new_x<0 || new_x>FFT_PRINT_SIZE)
				{
					wtf_buffer[y][x]=0;
					continue;
				};
				wtf_buffer[y][x]=wtf_buffer[y][new_x];
			}
		}
		if(freq_diff<0)
		{
			for (int16_t x = FFT_PRINT_SIZE; x >= 0; x--)
			{
				new_x=x+freq_diff;
				if(new_x<0 || new_x>FFT_PRINT_SIZE)
				{
					wtf_buffer[y][x]=0;
					continue;
				};
				wtf_buffer[y][x]=wtf_buffer[y][new_x];
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
