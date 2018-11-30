#include "stm32f4xx_hal.h"
#include "trx_manager.h"
#include "functions.h"
#include "lcd.h"
#include "fpga.h"
#include "settings.h"
#include "wm8731.h"
#include "fpga.h"
#include "bands.h"
#include "helper.h"

uint32_t TRX_freq_phrase = 0; //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;
bool TRX_ptt = 0;
bool TRX_tune = 0;
bool TRX_inited = false;
int32_t TRX_s_meter = 1;
bool TRX_agc_wdsp_action = 0;
bool TRX_ADC_OTR = 0;
uint16_t TRX_Filter_Width=2700;

char *MODE_DESCR[10] = {
	"LSB",
	"USB",
	"IQ",
	"CW",
	"DIGL",
	"DIGU",
	"NOTX",
	"FM",
	"AM",
	"LOOP"
};

void TRX_Init()
{
	TRX_freq_phrase = getPhraseFromFrequency(TRX.Freq); //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304; 7.100.000 // 618222-7.075.000 / 09 6e ee  / 9 110 238
	start_i2s();
}

void TRX_ptt_change()
{
	if (TRX_tune) return;
	bool TRX_new_ptt = !HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin);
	if (TRX_ptt != TRX_new_ptt)
	{
		TRX_ptt = TRX_new_ptt;
		LCD_displayStatusInfoGUI();
		FPGA_NeedSendParams = true;
		start_i2s();
		HELPER_updateSettings();
	}
}

void TRX_setFrequency(uint32_t _freq)
{
	if (_freq < 100) _freq = 100;
	if (_freq >= ADCDAC_CLOCK / 2) _freq = ADCDAC_CLOCK / 2;
	
	FFT_moveWaterfall(_freq-TRX.Freq);
	
	TRX.Freq = _freq;
	if (TRX.BandMapEnabled && TRX_getMode() != getModeFromFreq(TRX.Freq))
	{
		TRX_setMode(getModeFromFreq(TRX.Freq));
				switch(TRX_getMode())
		{
			case TRX_MODE_LSB:
			case TRX_MODE_USB:
			case TRX_MODE_DIGI_L:
			case TRX_MODE_DIGI_U:
			case TRX_MODE_AM:
				TRX_Filter_Width=TRX.SSB_Filter;
				break;
			case TRX_MODE_CW:
				TRX_Filter_Width=TRX.CW_Filter;
				break;
			case TRX_MODE_FM:
				TRX_Filter_Width=TRX.FM_Filter;
				break;
		}
		LCD_displayTopButtons(false);
	}
	FPGA_NeedSendParams = true;
}

int32_t TRX_getFrequency(void)
{
	return TRX.Freq;
}

void TRX_setMode(uint8_t _mode)
{
	if (TRX.Mode == TRX_MODE_LOOPBACK || _mode == TRX_MODE_LOOPBACK)
	{
		TRX.Mode = _mode;
		start_i2s();
	}
	else
	{
		TRX.Mode = _mode;
	}
	NeedSaveSettings = true;
}

uint8_t TRX_getMode(void)
{
	return TRX.Mode;
}


