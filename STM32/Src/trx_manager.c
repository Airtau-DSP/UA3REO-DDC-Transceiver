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
int32_t TRX_s_meter = 1;
bool TRX_agc_wdsp_action = 0;
bool TRX_ADC_OTR = 0;

void TRX_Init()
{
	TRX_freq_phrase = getPhraseFromFrequency(TRX.Freq); //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304; 7.100.000 // 618222-7.075.000 / 09 6e ee  / 9 110 238
	LCD_displayTopButtons(false);
	TRX_SetLoopbackMode(TRX.Loopback);
}

void TRX_ptt_change()
{
	TRX_ptt = !HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin);
	FPGA_NeedSendParams = true;
	start_i2s_dma();
	LCD_displayStatusInfoGUI();
}

void TRX_setFrequency(int32_t _freq)
{
	if (_freq < 100) _freq = 100;
	if (_freq >= 99999999) _freq = 99999999;
	TRX.Freq = _freq;
	TRX_freq_phrase = getPhraseFromFrequency(_freq);

	uint8_t tmp_packet = ((TRX_getFrequencyPhrase() & (0XFF << 16)) >> 16);
	if (TRX_getMode() == TRX_MODE_USB) bitWrite(tmp_packet, 7, 1); else bitWrite(tmp_packet, 7, 0);
	if (TRX_getMode() == TRX_MODE_IQ) bitWrite(tmp_packet, 6, 1); else bitWrite(tmp_packet, 6, 0);

	FPGA_NeedSendParams = true;
}

void TRX_setFrequencyPhrase(int32_t _phrase)
{
	TRX_freq_phrase = _phrase;
	TRX.Freq = getFrequencyFromPhrase(TRX_freq_phrase);

	FPGA_NeedSendParams = true;
}

int32_t TRX_getFrequency(void)
{
	return TRX.Freq;
}

int32_t TRX_getFrequencyPhrase(void)
{
	return TRX_freq_phrase;
}

void TRX_SetLoopbackMode(bool state)
{
	TRX.Loopback = state;
	start_i2s_dma();
}


void TRX_setMode(uint8_t _mode)
{
	TRX.Mode = _mode;
	//if (TRX_mode == TRX_MODE_USB || TRX_mode == TRX_MODE_LSB) TRX_hilbert=true; else TRX_hilbert=false;
	LCD_displayTopButtons(false);
	SaveSettings();
}

uint8_t TRX_getMode(void)
{
	return TRX.Mode;
}


