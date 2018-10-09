#include "stm32f4xx_hal.h"
#include "trx_manager.h"
#include "functions.h"
#include "lcd.h"
#include "fpga.h"

//uint32_t TRX_freq = 3635000;
uint32_t TRX_freq = 7078000;
//uint32_t TRX_freq = 7100000;
//uint32_t TRX_freq = 14076000;
uint32_t TRX_freq_phrase = 0; //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;
uint8_t TRX_mode = TRX_MODE_LSB;
bool TRX_preamp = true;
bool TRX_hilbert = false;
bool TRX_agc = true;
bool TRX_loopback = false;
uint8_t TRX_gain_level=10;
uint8_t TRX_agc_speed=2;
bool TRX_ptt=0;
bool TRX_tune=0;
int32_t TRX_s_meter=1;
bool TRX_agc_wdsp_action=0;

void TRX_Init()
{
  TRX_freq_phrase = getPhraseFromFrequency(TRX_freq); //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304; 7.100.000 // 618222-7.075.000 / 09 6e ee  / 9 110 238
  LCD_displayTopButtons(false);
}

void TRX_ptt_change()
{
  TRX_ptt = !HAL_GPIO_ReadPin(PTT_IN_GPIO_Port,PTT_IN_Pin);
  LCD_displayStatusInfoGUI();
}

void TRX_setFrequency(int32_t _freq)
{
  if (_freq < 100) _freq = 100;
  if (_freq >= 99999999) _freq = 99999999;
  TRX_freq = _freq;
	TRX_freq_phrase = getPhraseFromFrequency(_freq);
	
	uint8_t tmp_packet = ((TRX_getFrequencyPhrase() & (0XFF << 16)) >> 16);
  if (TRX_getMode() == TRX_MODE_USB) bitWrite(tmp_packet, 7, 1); else bitWrite(tmp_packet, 7, 0);
  if (TRX_getMode() == TRX_MODE_IQ) bitWrite(tmp_packet, 6, 1); else bitWrite(tmp_packet, 6, 0);
}

void TRX_setFrequencyPhrase(int32_t _phrase)
{
  TRX_freq_phrase = _phrase;
  TRX_freq = getFrequencyFromPhrase(TRX_freq_phrase);
}

int32_t TRX_getFrequency(void)
{
  return TRX_freq;
}

int32_t TRX_getFrequencyPhrase(void)
{
  return TRX_freq_phrase;
}


void TRX_setMode(uint8_t _mode)
{
  TRX_mode = _mode;
  //if (TRX_mode == TRX_MODE_USB || TRX_mode == TRX_MODE_LSB) TRX_hilbert=true; else TRX_hilbert=false;
  LCD_displayTopButtons(false);
}

uint8_t TRX_getMode(void)
{
  return TRX_mode;
}


