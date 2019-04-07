#include "stm32f4xx_hal.h"
#include "main.h"
#include "trx_manager.h"
#include "functions.h"
#include "lcd.h"
#include "fpga.h"
#include "settings.h"
#include "wm8731.h"
#include "fpga.h"
#include "bands.h"
#include "audio_filters.h"

uint32_t TRX_freq_phrase = 0; //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;

bool TRX_ptt_hard = false;
bool TRX_old_ptt_hard = false;
bool TRX_ptt_cat = false;
bool TRX_old_ptt_cat = false;
bool TRX_key_serial = false;
bool TRX_old_key_serial = false;
bool TRX_key_hard = false;
bool TRX_old_key_hard = false;
uint16_t TRX_Key_Timeout_est = 0;

bool TRX_IQ_swap = false;
bool TRX_squelched = false;
bool TRX_tune = false;
bool TRX_inited = false;
int16_t TRX_RX_dBm = -100;
bool TRX_agc_wdsp_action = false;
bool TRX_ADC_OTR = false;
bool TRX_DAC_OTR = false;
uint8_t TRX_Time_InActive = 0; //секунд бездействия, используется для спящего режима

char *MODE_DESCR[] = {
	"LSB",
	"USB",
	"IQ",
	"CW_L",
	"CW_U",
	"DIGL",
	"DIGU",
	"NOTX",
	"NFM",
	"WFM",
	"AM",
	"LOOP"
};

bool TRX_on_TX(void)
{
	if (TRX_ptt_hard || TRX_ptt_cat || TRX_tune || TRX_getMode() == TRX_MODE_LOOPBACK || TRX_Key_Timeout_est>0) return true;
	return false;
}

void TRX_Init()
{
	TRX_freq_phrase = getPhraseFromFrequency(CurrentVFO()->Freq); //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304; 7.100.000 // 618222-7.075.000 / 09 6e ee  / 9 110 238
	TRX_Start_RX();
	TRX_setMode(CurrentVFO()->Mode);
}

void TRX_Restart_Mode()
{
	if (TRX_on_TX())
	{
		if (TRX_getMode() == TRX_MODE_LOOPBACK)
			TRX_Start_Loopback();
		else
			TRX_Start_TX();
	}
	else
	{
		TRX_Start_RX();
	}
}

void TRX_Start_RX()
{
	sendToDebug_str("RX MODE\r\n");
	memset(&CODEC_Audio_Buffer_RX[0], 0x00, CODEC_AUDIO_BUFFER_SIZE * 4);
	Processor_NeedRXBuffer = true;
	FPGA_Audio_Buffer_Index = 0;
	WM8731_Buffer_underrun = false;
	WM8731_DMA_state = true;
	WM8731_RX_mode();
	WM8731_start_i2s_and_dma();
}

void TRX_Start_TX()
{
	sendToDebug_str("TX MODE\r\n");
	memset(&CODEC_Audio_Buffer_RX[0], 0x00, CODEC_AUDIO_BUFFER_SIZE * 4);
	memset(&CODEC_Audio_Buffer_TX[0], 0x00, CODEC_AUDIO_BUFFER_SIZE * 4);
	HAL_Delay(10); //задерка перед подачей ВЧ сигнала, чтобы успели сработать реле
	WM8731_TX_mode();
	WM8731_start_i2s_and_dma();
}

void TRX_Start_Loopback()
{
	sendToDebug_str("LOOP MODE\r\n");
	memset(&CODEC_Audio_Buffer_RX[0], 0x00, CODEC_AUDIO_BUFFER_SIZE * 4);
	memset(&CODEC_Audio_Buffer_TX[0], 0x00, CODEC_AUDIO_BUFFER_SIZE * 4);
	WM8731_TXRX_mode();
	WM8731_start_i2s_and_dma();
}

void TRX_ptt_change(void)
{
	if (TRX_tune) return;
	bool TRX_new_ptt_hard = !HAL_GPIO_ReadPin(PTT_IN_GPIO_Port, PTT_IN_Pin);
	if (TRX_ptt_hard != TRX_new_ptt_hard)
	{
		TRX_Time_InActive=0;
		TRX_ptt_hard = TRX_new_ptt_hard;
		TRX_ptt_cat=false;
		LCD_displayStatusInfoGUI();
		FPGA_NeedSendParams = true;
		TRX_Restart_Mode();
	}
	if (TRX_ptt_cat != TRX_old_ptt_cat)
	{
		TRX_Time_InActive=0;
		TRX_old_ptt_cat=TRX_ptt_cat;
		LCD_displayStatusInfoGUI();
		FPGA_NeedSendParams = true;
		TRX_Restart_Mode();
	}
}

void TRX_key_change(void)
{
	if (TRX_tune) return;
	bool TRX_new_ptt_hard = !HAL_GPIO_ReadPin(KEY_IN_GPIO_Port, KEY_IN_Pin);
	if (TRX_key_hard != TRX_new_ptt_hard)
	{
		TRX_Time_InActive=0;
		TRX_key_hard=TRX_new_ptt_hard;
		if(TRX_key_hard==true) TRX_Key_Timeout_est=TRX.Key_timeout;
		if(TRX.Key_timeout==0) TRX_ptt_cat=TRX_key_hard;
		LCD_displayStatusInfoGUI();
		FPGA_NeedSendParams = true;
		TRX_Restart_Mode();
	}
	if (TRX_key_serial != TRX_old_key_serial)
	{
		TRX_Time_InActive=0;
		TRX_old_key_serial=TRX_key_serial;
		if(TRX_key_serial==true) TRX_Key_Timeout_est=TRX.Key_timeout;
		if(TRX.Key_timeout==0) TRX_ptt_cat=TRX_key_serial;
		LCD_displayStatusInfoGUI();
		FPGA_NeedSendParams = true;
		TRX_Restart_Mode();
	}
}

void TRX_setFrequency(int32_t _freq)
{
	if (_freq < 1) return;
	if (_freq >= MAX_FREQ_HZ) _freq = MAX_FREQ_HZ;

	FFT_moveWaterfall(_freq - CurrentVFO()->Freq);

	CurrentVFO()->Freq = _freq;
	if(getBandFromFreq(_freq)>=0) TRX.saved_freq[getBandFromFreq(_freq)]=_freq;
	if (TRX.BandMapEnabled && TRX_getMode() != getModeFromFreq(CurrentVFO()->Freq))
	{
		TRX_setMode(getModeFromFreq(CurrentVFO()->Freq));
		switch (TRX_getMode())
		{
		case TRX_MODE_LSB:
		case TRX_MODE_USB:
		case TRX_MODE_DIGI_L:
		case TRX_MODE_DIGI_U:
		case TRX_MODE_AM:
			CurrentVFO()->Filter_Width = TRX.SSB_Filter;
			break;
		case TRX_MODE_CW_L:
		case TRX_MODE_CW_U:
			CurrentVFO()->Filter_Width = TRX.CW_Filter;
			break;
		case TRX_MODE_NFM:
			CurrentVFO()->Filter_Width = TRX.FM_Filter;
			break;
		case TRX_MODE_WFM:
			CurrentVFO()->Filter_Width = 0;
			break;
		}
		InitFilters();
		LCD_displayTopButtons(false);
	}
	FPGA_NeedSendParams = true;
}

int32_t TRX_getFrequency(void)
{
	return CurrentVFO()->Freq;
}

void TRX_setMode(uint8_t _mode)
{
	if (CurrentVFO()->Mode == TRX_MODE_LOOPBACK || _mode == TRX_MODE_LOOPBACK)
	{
		CurrentVFO()->Mode = _mode;
		TRX_Start_Loopback();
	}
	else
	{
		CurrentVFO()->Mode = _mode;
	}
	NeedSaveSettings = true;
}

uint8_t TRX_getMode(void)
{
	return CurrentVFO()->Mode;
}


