#ifndef FFT_h
#define FFT_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <math.h> 
#include "arm_math.h"

#define FFT_SIZE 256 //указываем размер FFT
#define FFT_MAX_HEIGHT 30 //указываем максимальную высоту FFT
#define FFT_BOTTOM_OFFSET 190 //указываем максимальную высоту FFT
#define FFT_WTF_HEIGHT 50 //указываем максимальную высоту водопада
#define FFT_CONTRAST 100 //минимальный порог сигнала FFT

extern void FFT_doFFT(void);
extern void FFT_printFFT(void);

extern uint32_t FFT_buff_index;
extern bool FFT_need_fft;
extern float32_t FFTInput[FFT_SIZE*2];
uint16_t getFFTColor(uint8_t height);
	
#endif
