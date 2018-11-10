#include "stm32f4xx_hal.h"
#include "trx_manager.h"
#include "functions.h"
#include "lcd.h"
#include "fpga.h"
#include "settings.h"
#include "wm8731.h"
#include "fpga.h"

uint32_t TRX_freq_phrase = 0; //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;
bool TRX_ptt = 0;
bool TRX_tune = 0;
bool TRX_inited = false;
int32_t TRX_s_meter = 1;
bool TRX_agc_wdsp_action = 0;
bool TRX_ADC_OTR = 0;

void TRX_Init()
{
	TRX_freq_phrase = getPhraseFromFrequency(TRX.Freq); //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304; 7.100.000 // 618222-7.075.000 / 09 6e ee  / 9 110 238
	TRX_SetLoopbackMode(TRX.Loopback);
}

void TRX_ptt_change()
{
	if(TRX_tune) return;
	bool TRX_new_ptt = !HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin);
	if(TRX_ptt!=TRX_new_ptt)
	{
		TRX_ptt=TRX_new_ptt;
		LCD_displayStatusInfoGUI();
		FPGA_NeedSendParams = true;
		start_i2s();
	}
}

void TRX_setFrequency(int32_t _freq)
{
	if (_freq < 100) _freq = 100;
	if (_freq >= 99999999) _freq = 99999999;
	TRX.Freq = _freq;
	FPGA_NeedSendParams = true;
}

int32_t TRX_getFrequency(void)
{
	return TRX.Freq;
}

void TRX_SetLoopbackMode(bool state)
{
	TRX.Loopback = state;
	start_i2s();
}


void TRX_setMode(uint8_t _mode)
{
	TRX.Mode = _mode;
	//if (TRX_mode == TRX_MODE_USB || TRX_mode == TRX_MODE_LSB) TRX_hilbert=true; else TRX_hilbert=false;
	NeedSaveSettings=true;
}

uint8_t TRX_getMode(void)
{
	return TRX.Mode;
}


