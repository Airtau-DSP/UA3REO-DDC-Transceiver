#include "fft.h"
#include "lcd.h"
#include <stdlib.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "functions.h"
#include "audio_processor.h"
#include "wm8731.h"
#include "settings.h"
#include "audio_filters.h"

#if FFT_SIZE==512
const static arm_cfft_instance_f32 *FFT_Inst = &arm_cfft_sR_f32_len512;
#endif
#if FFT_SIZE==256
const static arm_cfft_instance_f32 *FFT_Inst = &arm_cfft_sR_f32_len256;
#endif

bool NeedFFTInputBuffer = false; //флаг необходимости заполнения буфера с FPGA
bool FFT_need_fft = true; //необходимо подготовить данные для отображения на экран

volatile uint32_t FFT_buff_index = 0; //текущий индекс буфера при его наполнении с FPGA
float32_t FFTInput_I[FFT_SIZE] = { 0 }; //входящий буфер FFT I
float32_t FFTInput_Q[FFT_SIZE] = { 0 }; //входящий буфер FFT Q
static float32_t FFTInput[FFT_DOUBLE_SIZE_BUFFER] = {0}; //совмещённый буфер FFT I и Q
static float32_t FFTInput_ZOOMFFT[FFT_DOUBLE_SIZE_BUFFER] = {0}; //совмещённый буфер FFT I и Q для обработки ZoomFFT
static float32_t FFTOutput_mean[FFT_PRINT_SIZE] = { 0 }; //усредненный буфер FFT (для вывода)
static uint16_t wtf_buffer[FFT_WTF_HEIGHT][FFT_PRINT_SIZE] = { {0} }; //буфер водопада
static uint16_t maxValueErrors = 0; //количество превышений сигнала в FFT
static float32_t maxValueFFT = 0; //максимальное значение амплитуды в результирующей АЧХ
static uint32_t currentFFTFreq = 0;
static uint16_t color_scale[FFT_MAX_HEIGHT] = { 0 }; //цветовой градиент по высоте FFT

//Дециматор для Zoom FFT
static arm_fir_decimate_instance_f32	DECIMATE_ZOOM_FFT_I;
static arm_fir_decimate_instance_f32	DECIMATE_ZOOM_FFT_Q;
static float32_t decimZoomFFTIState[FFT_SIZE + 4 - 1];
static float32_t decimZoomFFTQState[FFT_SIZE + 4 - 1];
static uint16_t zoomed_width = 0;

static uint16_t getFFTColor(uint8_t height);
static void fft_fill_color_scale(void);

//Коэффициенты для ZoomFFT lowpass filtering / дециматора
static arm_biquad_casd_df1_inst_f32 IIR_biquad_Zoom_FFT_I =
{
	.numStages = 4,
	.pCoeffs = (float32_t *)(float32_t [])
	{
		1,0,0,0,0,  1,0,0,0,0
  },
  .pState = (float32_t *)(float32_t [])
	{
		0,0,0,0,   0,0,0,0,    0,0,0,0,   0,0,0,0
	}
};
static arm_biquad_casd_df1_inst_f32 IIR_biquad_Zoom_FFT_Q =
{
	.numStages = 4,
	.pCoeffs = (float32_t *)(float32_t [])
	{
		1,0,0,0,0,  1,0,0,0,0
  },
  .pState = (float32_t *)(float32_t [])
	{
		0,0,0,0,   0,0,0,0,    0,0,0,0,   0,0,0,0
	}
};

static const float32_t* mag_coeffs[17] =
{
	NULL, // 0
	NULL, // 1
	(float32_t*)(const float32_t[]) {
		// 2x magnify
		// 12kHz, sample rate 48k, 60dB stopband, elliptic
		// a1 and coeffs[A2] negated! order: coeffs[B0], coeffs[B1], coeffs[B2], a1, coeffs[A2]
		// Iowa Hills IIR Filter Designer, DD4WH Aug 16th 2016
		0.228454526413293696,0.077639329099949764,0.228454526413293696,0.635534925142242080,-0.170083307068779194,
		0.436788292542003964,0.232307972937606161,0.436788292542003964,0.365885230717786780,-0.471769788739400842,
		0.535974654742658707,0.557035600464780845,0.535974654742658707,0.125740787233286133,-0.754725697183384336,
		0.501116342273565607,0.914877831284765408,0.501116342273565607,0.013862536615004284,-0.930973052446900984
	},
	NULL, // 3
	(float32_t*)(const float32_t[])
	{
		// 4x magnify
		// 6kHz, sample rate 48k, 60dB stopband, elliptic
		// a1 and coeffs[A2] negated! order: coeffs[B0], coeffs[B1], coeffs[B2], a1, coeffs[A2]
		// Iowa Hills IIR Filter Designer, DD4WH Aug 16th 2016
		0.182208761527446556,-0.222492493114674145,0.182208761527446556,1.326111070880959810,-0.468036100821178802,
		0.337123762652097259,-0.366352718812586853,0.337123762652097259,1.337053579516321200,-0.644948386007929031,
		0.336163175380826074,-0.199246162162897811,0.336163175380826074,1.354952684569386670,-0.828032873168141115,
		0.178588201750411041,0.207271695028067304,0.178588201750411041,1.386486967455699220,-0.950935065984588657
	},
	NULL, // 5
	NULL, // 6
	NULL, // 7
	(float32_t*)(const float32_t[])
	{
		// 8x magnify
		// 3kHz, sample rate 48k, 60dB stopband, elliptic
		// a1 and coeffs[A2] negated! order: coeffs[B0], coeffs[B1], coeffs[B2], a1, coeffs[A2]
		// Iowa Hills IIR Filter Designer, DD4WH Aug 16th 2016
		0.185643392652478922,-0.332064345389014803,0.185643392652478922,1.654637402827731090,-0.693859842743674182,
		0.327519300813245984,-0.571358085216950418,0.327519300813245984,1.715375037176782860,-0.799055553586324407,
		0.283656142708241688,-0.441088976843048652,0.283656142708241688,1.778230635987093860,-0.904453944560528522,
		0.079685368654848945,-0.011231810140649204,0.079685368654848945,1.825046003243238070,-0.973184930412286708
	},
	NULL, // 9
	NULL, // 10
	NULL, // 11
	NULL, // 12
	NULL, // 13
	NULL, // 14
	NULL, // 15
	(float32_t*)(const float32_t[])
	{
		// 16x magnify
		// 1k5, sample rate 48k, 60dB stopband, elliptic
		// a1 and coeffs[A2] negated! order: coeffs[B0], coeffs[B1], coeffs[B2], a1, coeffs[A2]
		// Iowa Hills IIR Filter Designer, DD4WH Aug 16th 2016
		0.194769868656866380,-0.379098413160710079,0.194769868656866380,1.824436402073870810,-0.834877726226893380,
		0.333973874901496770,-0.646106479315673776,0.333973874901496770,1.871892825636887640,-0.893734096124207178,
		0.272903880596429671,-0.513507745397738469,0.272903880596429671,1.918161772571113750,-0.950461788366234739,
		0.053535383722369843,-0.069683422367188122,0.053535383722369843,1.948900719896301760,-0.986288064973853129
	},
};

static const arm_fir_decimate_instance_f32 FirZoomFFTDecimate[17] =
{
	{NULL}, // 0
	{NULL}, // 1
	// 48ksps, 12kHz lowpass
	{
		.numTaps = 4,
		.pCoeffs = (float*) (const float[])
		{475.1179397144384210E-6,0.503905202786044337,0.503905202786044337,475.1179397144384210E-6},
		.pState = NULL
	},
	{NULL}, // 3
	// 48ksps, 6kHz lowpass
	{
		.numTaps = 4,
		.pCoeffs = (float*) (const float[])
		{0.198273254218889416,0.298085149879260325,0.298085149879260325,0.198273254218889416},
		.pState = NULL
	},
	{NULL}, // 5
	{NULL}, // 6
	{NULL}, // 7
	// 48ksps, 3kHz lowpass
	{
		.numTaps = 4,
		.pCoeffs = (float*) (const float[])
		{0.199820836596682871,0.272777397353925699,0.272777397353925699,0.199820836596682871},
		.pState = NULL
	},
	{NULL}, // 9
	{NULL}, // 10
	{NULL}, // 11
	{NULL}, // 12
	{NULL}, // 13
	{NULL}, // 14
	{NULL}, // 15
	// 48ksps, 1.5kHz lowpass
	{
		.numTaps = 4,
		.pCoeffs = (float*) (const float[])
		{0.199820836596682871,0.272777397353925699,0.272777397353925699,0.199820836596682871},
		.pState = NULL
	},
};

void FFT_Init(void)
{
	fft_fill_color_scale();
	//ZoomFFT
	if(TRX.FFT_Zoom>1)
	{
		IIR_biquad_Zoom_FFT_I.pCoeffs = mag_coeffs[TRX.FFT_Zoom];
		IIR_biquad_Zoom_FFT_Q.pCoeffs = mag_coeffs[TRX.FFT_Zoom];
		arm_fir_decimate_init_f32(&DECIMATE_ZOOM_FFT_I,
							FirZoomFFTDecimate[TRX.FFT_Zoom].numTaps,
							TRX.FFT_Zoom,          // Decimation factor
							FirZoomFFTDecimate[TRX.FFT_Zoom].pCoeffs,
							decimZoomFFTIState,            // Filter state variables
							FFT_SIZE);

		arm_fir_decimate_init_f32(&DECIMATE_ZOOM_FFT_Q,
							FirZoomFFTDecimate[TRX.FFT_Zoom].numTaps,
							TRX.FFT_Zoom,          // Decimation factor
							FirZoomFFTDecimate[TRX.FFT_Zoom].pCoeffs,
							decimZoomFFTQState,            // Filter state variables
							FFT_SIZE);
		zoomed_width=FFT_SIZE/TRX.FFT_Zoom;
	}
}

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
	
	//Process DC corrector filter
	dc_filter(FFTInput_I,FFT_SIZE, 4);
	dc_filter(FFTInput_Q,FFT_SIZE, 5);
	
	//Process Notch filter
	arm_biquad_cascade_df2T_f32(&NOTCH_FILTER_FFT_I, FFTInput_I, FFTInput_I, FFT_SIZE);
	arm_biquad_cascade_df2T_f32(&NOTCH_FILTER_FFT_Q, FFTInput_Q, FFTInput_Q, FFT_SIZE);
	
	//ZoomFFT
	if(TRX.FFT_Zoom>1)
	{
		//Biquad LPF фильтр
		arm_biquad_cascade_df1_f32 (&IIR_biquad_Zoom_FFT_I, FFTInput_I, FFTInput_I, FFT_SIZE);
		arm_biquad_cascade_df1_f32 (&IIR_biquad_Zoom_FFT_Q, FFTInput_Q, FFTInput_Q, FFT_SIZE);
		//Дециматор
		arm_fir_decimate_f32(&DECIMATE_ZOOM_FFT_I, FFTInput_I, FFTInput_I, FFT_SIZE);
    arm_fir_decimate_f32(&DECIMATE_ZOOM_FFT_Q, FFTInput_Q, FFTInput_Q, FFT_SIZE);
		//Смещаем старые данные в  буфере, т.к. будем их использовать (иначе скорость FFT упадёт, ведь для получения большего разрешения необходимо накапливать больше данных)
		for (uint16_t i = 0; i < FFT_SIZE; i++)
		{
			if(i<(FFT_SIZE-zoomed_width))
			{
				FFTInput_ZOOMFFT[i*2]=FFTInput_ZOOMFFT[(i+zoomed_width)*2];
				FFTInput_ZOOMFFT[i*2 + 1]=FFTInput_ZOOMFFT[(i+zoomed_width)*2 + 1];
			}
			else //Добавляем новые данные в буфер FFT для расчёта
			{
				FFTInput_ZOOMFFT[i*2] = FFTInput_I[i-(FFT_SIZE-zoomed_width)];
				FFTInput_ZOOMFFT[i*2 + 1] = FFTInput_Q[i-(FFT_SIZE-zoomed_width)];
			}
			FFTInput[i * 2] = FFTInput_ZOOMFFT[i * 2];
			FFTInput[i * 2 + 1] = FFTInput_ZOOMFFT[i * 2 + 1];
		}
	}
	else
	{
		//делаем совмещённый буфер для расчёта
		for (uint16_t i = 0; i < FFT_SIZE; i++)
		{
			FFTInput[i * 2] = FFTInput_I[i];
			FFTInput[i * 2 + 1] = FFTInput_Q[i];
		}
	}
	
	//Окно Hanning
	for (uint16_t i = 0; i < FFT_SIZE; i++)
	{
		hanning_multiplier = 0.5f * (1.0f - arm_cos_f32(2.0f * PI*i / FFT_SIZE*2));
		FFTInput[i * 2] = hanning_multiplier * FFTInput[i * 2];
		FFTInput[i * 2 + 1] = hanning_multiplier * FFTInput[i * 2 + 1];
	}
	
	arm_cfft_f32(FFT_Inst, FFTInput, 0, 1);
	arm_cmplx_mag_f32(FFTInput, FFTInput, FFT_SIZE);
	
	//Уменьшаем расчитанный FFT до видимого
	if(FFT_SIZE>FFT_PRINT_SIZE)
	{
		uint8_t fft_compress_rate=FFT_SIZE / FFT_PRINT_SIZE;
		for(uint16_t i=0;i<FFT_PRINT_SIZE;i++)
		{
			float32_t fft_compress_tmp = 0;
			for (uint8_t c = 0; c < fft_compress_rate; c++)
				fft_compress_tmp += FFTInput[i*fft_compress_rate + c];
			FFTInput[i] = fft_compress_tmp / fft_compress_rate;
		}
	}
	
	FFTInput[0]=FFTInput[1];
	
	//Автокалибровка уровней FFT
	arm_max_f32(FFTInput, FFT_PRINT_SIZE, &maxValue, &maxIndex); //ищем максимум в АЧХ
	arm_mean_f32(FFTInput, FFT_PRINT_SIZE, &meanValue); //ищем среднее в АЧХ
	diffValue=(maxValue-maxValueFFT)/FFT_STEP_COEFF;
	if (maxValueErrors >= FFT_MAX_IN_RED_ZONE && diffValue>0) maxValueFFT+=diffValue;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && diffValue<0 && diffValue<-FFT_STEP_FIX) maxValueFFT+=diffValue;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && maxValueFFT>FFT_STEP_FIX) maxValueFFT-=FFT_STEP_FIX;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && diffValue<0 && diffValue<-FFT_STEP_PRECISION) maxValueFFT+=diffValue;
	else if (maxValueErrors <= FFT_MIN_IN_RED_ZONE && maxValueFFT>FFT_STEP_PRECISION) maxValueFFT-=FFT_STEP_PRECISION;
	if((meanValue*4)>maxValueFFT) maxValueFFT=(meanValue*4);
	maxValueErrors = 0;
	if (maxValueFFT < FFT_MIN) maxValueFFT = FFT_MIN;
	if(TRX_getMode()==TRX_MODE_LOOPBACK) maxValueFFT=60000;
	
	//Нормируем АЧХ к единице
	arm_scale_f32(FFTInput,1.0f/maxValueFFT,FFTInput,FFT_PRINT_SIZE);
	
	//Усреднение значений для последующего вывода (от резких всплесков)
	for(uint16_t i=0;i<FFT_PRINT_SIZE;i++)
		if(FFTOutput_mean[i]<FFTInput[i])
			FFTOutput_mean[i]+=(FFTInput[i]-FFTOutput_mean[i])/TRX.FFT_Averaging;
		else
			FFTOutput_mean[i]-=(FFTOutput_mean[i]-FFTInput[i])/TRX.FFT_Averaging;

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
	
	uint16_t height = 0; //высота столбца в выводе FFT
	uint16_t tmp = 0;
	
	//смещаем водопад, если нужно
	if((CurrentVFO()->Freq - currentFFTFreq) != 0)
	{
		FFT_moveWaterfall(CurrentVFO()->Freq - currentFFTFreq);
		currentFFTFreq = CurrentVFO()->Freq;
	}
	
	//смещаем водопад вниз c помощью DMA
	for (tmp = FFT_WTF_HEIGHT - 1; tmp > 0; tmp--)
	{
		HAL_DMA_Start(&hdma_memtomem_dma2_stream7, (uint32_t)&wtf_buffer[tmp - 1], (uint32_t)&wtf_buffer[tmp], FFT_PRINT_SIZE/2);
		HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream7, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}

	//расчитываем цвета для водопада
	uint8_t new_x = 0;
	uint8_t fft_header[FFT_PRINT_SIZE]={0};
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
		fft_header[new_x]=height;
		if(new_x==(FFT_PRINT_SIZE / 2)) continue;
	}
	
	//выводим FFT над водопадом
	LCDDriver_SetCursorAreaPosition(0, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT, FFT_PRINT_SIZE-1, FFT_BOTTOM_OFFSET);
	for (uint32_t fft_y = 0; fft_y < FFT_MAX_HEIGHT; fft_y++)
	for (uint32_t fft_x = 0; fft_x < FFT_PRINT_SIZE; fft_x++)
	{
		if(new_x==(FFT_PRINT_SIZE / 2)) continue;
		if(fft_y<(FFT_MAX_HEIGHT-fft_header[fft_x]))
			LCDDriver_SendData(COLOR_BLACK);
		else
			LCDDriver_SendData(color_scale[fft_y]);
	}
	
	//разделитель и полоса приёма
	LCDDriver_drawFastVLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET, -FFT_MAX_HEIGHT, COLOR_GREEN);
	LCDDriver_drawFastHLine(0, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, FFT_PRINT_SIZE, COLOR_BLACK);
	uint16_t line_width=0;
	line_width=CurrentVFO()->Filter_Width/FFT_HZ_IN_PIXEL*TRX.FFT_Zoom;
	switch(CurrentVFO()->Mode)
	{
		case TRX_MODE_LSB:
		case TRX_MODE_CW_L:
		case TRX_MODE_DIGI_L:
			if(line_width>(FFT_PRINT_SIZE / 2)) line_width=FFT_PRINT_SIZE / 2;
			LCDDriver_drawFastHLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, -line_width, COLOR_GREEN);
			break;
		case TRX_MODE_USB:
		case TRX_MODE_CW_U:
		case TRX_MODE_DIGI_U:
			if(line_width>(FFT_PRINT_SIZE / 2)) line_width=FFT_PRINT_SIZE / 2;
			LCDDriver_drawFastHLine(FFT_PRINT_SIZE / 2, FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, line_width , COLOR_GREEN);
			break;
		case TRX_MODE_NFM:
		case TRX_MODE_AM:
			if(line_width>FFT_PRINT_SIZE) line_width=FFT_PRINT_SIZE;
			LCDDriver_drawFastHLine((FFT_PRINT_SIZE / 2)-(line_width/2), FFT_BOTTOM_OFFSET-FFT_MAX_HEIGHT-2, line_width, COLOR_GREEN);
			break;
		default:
			break;
	}
	
	//выводим на экран водопада с помощью DMA
	LCDDriver_SetCursorAreaPosition(0, FFT_BOTTOM_OFFSET, FFT_PRINT_SIZE-1, FFT_BOTTOM_OFFSET + FFT_WTF_HEIGHT);
	HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream6, (uint32_t)&wtf_buffer, LCD_FSMC_DATA_ADDR, FFT_WTF_HEIGHT*FFT_PRINT_SIZE);
}

void FFT_moveWaterfall(int16_t freq_diff)
{
	int16_t new_x = 0;
	freq_diff = (freq_diff / FFT_HZ_IN_PIXEL) * TRX.FFT_Zoom;
	for (uint8_t y = 0; y < FFT_WTF_HEIGHT; y++)
	{
		if (freq_diff > 0) //freq up
		{
			for (int16_t x = 0; x < FFT_PRINT_SIZE; x++)
			{
				new_x = x + freq_diff;
				if (new_x<0 || new_x>=FFT_PRINT_SIZE)
				{
					wtf_buffer[y][x] = 0;
					FFTOutput_mean[x] = 0;
					continue;
				};
				wtf_buffer[y][x] = wtf_buffer[y][new_x];
				if(y==0) FFTOutput_mean[x]=FFTOutput_mean[new_x];
			}
		}
		else if (freq_diff < 0) // freq down
		{
			for (int16_t x = FFT_PRINT_SIZE-1; x >= 0; x--)
			{
				new_x = x + freq_diff;
				if (new_x<0)
				{
					wtf_buffer[y][x] = 0;
					FFTOutput_mean[x] = 0;
					continue;
				};
				wtf_buffer[y][x] = wtf_buffer[y][new_x];
				if(y==0) FFTOutput_mean[x]=FFTOutput_mean[new_x];
			}
		}
	}
}

static uint16_t getFFTColor(uint8_t height) //получение теплоты цвета FFT (от синего к красному)
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
		red = 255;
		blue = 0;
		green = 255 - ((height - 2 * FFT_MAX_HEIGHT / 3) * 255 / (FFT_MAX_HEIGHT / 3));
	}
	return rgb888torgb565(red, green, blue);
}

static void fft_fill_color_scale(void) //заполняем градиент цветов FFT при инициализации
{
	for (uint8_t i=0;i<FFT_MAX_HEIGHT;i++) 
  {
		color_scale[i]=getFFTColor(FFT_MAX_HEIGHT-i);
	}
}
