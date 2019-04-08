#ifndef FFT_h
#define FFT_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <math.h> 
#include "arm_math.h"
#include "wm8731.h"

#define FFT_SIZE 512 //указываем размер расчитываемого FFT
#define FFT_PRINT_SIZE 256 //указываем размер выводимого FFT
#define FFT_MAX_HEIGHT 30 //указываем максимальную высоту FFT
#define FFT_BOTTOM_OFFSET 190 //start of FFT
#define FFT_WTF_HEIGHT 50 //указываем максимальную высоту водопада
#define FFT_MIN 20.0f //MIN порог сигнала FFT
#define FFT_STEP_COEFF 10.0f //коэффициент шага автокалибровки сигнала FFT (больше - медленней)
#define FFT_STEP_FIX 10.0f //шаг снижения коэффициента FFT
#define FFT_STEP_PRECISION 1.0f //шаг снижения коэффициента FFT (для слабых сигналов)
#define FFT_MAX_IN_RED_ZONE 8 //максимум красных пиков на водопаде (для автоподстройки)
#define FFT_MIN_IN_RED_ZONE 1 //минимум красных пиков на водопаде (для автоподстройки)
#define FFT_HZ_IN_PIXEL (WM8731_SAMPLERATE/FFT_PRINT_SIZE) // герц в одном пикселе

extern void FFT_doFFT(void);
extern void FFT_printFFT(void);
extern void FFT_moveWaterfall(int16_t freq_diff);

extern uint32_t FFT_buff_index;
extern bool FFTInputBufferInProgress;
extern bool NeedFFTInputBuffer;
extern bool FFT_need_fft;
extern float32_t FFTInput[FFT_SIZE * 2];
uint16_t getFFTColor(uint8_t height);

extern DMA_HandleTypeDef hdma_memtomem_dma2_stream6;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream7;

#endif
