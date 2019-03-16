#ifndef FFT_h
#define FFT_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <math.h> 
#include "arm_math.h"

#define FFT_SIZE 512 //указываем размер FFT
#define FFT_PRINT_SIZE 256 //указываем размер FFT
#define FFT_MAX_HEIGHT 30 //указываем максимальную высоту FFT
#define FFT_BOTTOM_OFFSET 190 //start of FFT
#define FFT_WTF_HEIGHT 50 //указываем максимальную высоту водопада
#define FFT_MIN 20.0f //MIN порог сигнала FFT
#define FFT_STEP_COEFF 10.0f //коэффициент шага автокалибровки сигнала FFT (больше - медленней)
#define FFT_STEP_FIX 50.0f //шаг снижения коэффициента FFT
#define FFT_STEP_PRECISION 10.0f //шаг снижения коэффициента FFT (для слабых сигналов)
#define FFT_MAX_IN_RED_ZONE 8 //максимум красных пиков на водопаде (для автоподстройки)
#define FFT_MIN_IN_RED_ZONE 3 //минимум красных пиков на водопаде (для автоподстройки)
#define FFT_HZ_IN_PIXEL (48000/FFT_PRINT_SIZE) // герц в одном пикселе

extern void FFT_doFFT(void);
extern void FFT_printFFT(void);
extern void FFT_moveWaterfall(int16_t freq_diff);

extern uint32_t FFT_buff_index;
extern bool FFTInputBufferInProgress;
extern bool FFT_need_fft;
extern float32_t FFTInput_A[FFT_SIZE * 2];
extern float32_t FFTInput_B[FFT_SIZE * 2];
uint16_t getFFTColor(uint8_t height);

#endif
