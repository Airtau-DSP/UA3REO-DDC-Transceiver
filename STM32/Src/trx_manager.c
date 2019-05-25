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
#include "usbd_audio_if.h"

volatile bool TRX_ptt_hard = false;
volatile bool TRX_old_ptt_hard = false;
volatile bool TRX_ptt_cat = false;
volatile bool TRX_old_ptt_cat = false;
volatile bool TRX_key_serial = false;
volatile bool TRX_old_key_serial = false;
volatile bool TRX_key_hard = false;
volatile bool TRX_old_key_hard = false;
volatile uint16_t TRX_Key_Timeout_est = 0;
volatile bool TRX_IQ_swap = false;
volatile bool TRX_squelched = false;
volatile bool TRX_tune = false;
volatile bool TRX_inited = false;
volatile int16_t TRX_RX_dBm = -100;
volatile bool TRX_ADC_OTR = false;
volatile bool TRX_DAC_OTR = false;
volatile uint8_t TRX_Time_InActive = 0; //секунд бездействия, используется для спящего режима
volatile int16_t TRX_ADC_MINAMPLITUDE = 0;
volatile int16_t TRX_ADC_MAXAMPLITUDE = 0;

uint8_t autogain_wait_reaction = 0; //таймер ожидания реакции от смены режимов
uint8_t autogain_stage = 0; //этап отработки актокорректировщика усиления

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
	TRX_RF_UNIT_UpdateState(false);
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
	TRX_RF_UNIT_UpdateState(false);
	memset(&CODEC_Audio_Buffer_RX[0], 0x00, CODEC_AUDIO_BUFFER_SIZE * 4);
	memset(&CODEC_Audio_Buffer_TX[0], 0x00, CODEC_AUDIO_BUFFER_SIZE * 4);
	HAL_Delay(10); //задерка перед подачей ВЧ сигнала, чтобы успели сработать реле
	WM8731_TX_mode();
	WM8731_start_i2s_and_dma();
}

void TRX_Start_Loopback()
{
	sendToDebug_str("LOOP MODE\r\n");
	TRX_RF_UNIT_UpdateState(false);
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
		LCD_UpdateQuery.StatusInfoGUI = true;
		FPGA_NeedSendParams = true;
		TRX_Restart_Mode();
	}
	if (TRX_ptt_cat != TRX_old_ptt_cat)
	{
		TRX_Time_InActive=0;
		TRX_old_ptt_cat=TRX_ptt_cat;
		LCD_UpdateQuery.StatusInfoGUI = true;
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
		LCD_UpdateQuery.StatusInfoGUI = true;
		FPGA_NeedSendParams = true;
		TRX_Restart_Mode();
	}
	if (TRX_key_serial != TRX_old_key_serial)
	{
		TRX_Time_InActive=0;
		TRX_old_key_serial=TRX_key_serial;
		if(TRX_key_serial==true) TRX_Key_Timeout_est=TRX.Key_timeout;
		if(TRX.Key_timeout==0) TRX_ptt_cat=TRX_key_serial;
		LCD_UpdateQuery.StatusInfoGUI = true;
		FPGA_NeedSendParams = true;
		TRX_Restart_Mode();
	}
}

void TRX_setFrequency(int32_t _freq)
{
	if (_freq < 1) return;
	if (_freq >= MAX_FREQ_HZ) _freq = MAX_FREQ_HZ;

	CurrentVFO()->Freq = _freq;
	if(getBandFromFreq(_freq)>=0) TRX.saved_freq[getBandFromFreq(_freq)]=_freq;
	if (TRX.BandMapEnabled)
	{
		uint8_t mode_from_bandmap = getModeFromFreq(CurrentVFO()->Freq);
		if (TRX_getMode() != mode_from_bandmap)
		{
			TRX_setMode(mode_from_bandmap);
			LCD_UpdateQuery.TopButtons = true;
		}
	}
	FPGA_NeedSendParams = true;
	FFT_buff_index = 0;
	NeedFFTInputBuffer = true;
}

int32_t TRX_getFrequency(void)
{
	return CurrentVFO()->Freq;
}

void TRX_setMode(uint8_t _mode)
{
	CurrentVFO()->Mode = _mode;
	if (CurrentVFO()->Mode == TRX_MODE_LOOPBACK || _mode == TRX_MODE_LOOPBACK) TRX_Start_Loopback();
	switch (_mode)
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
	ReinitAudioFilters();
	NeedSaveSettings = true;
}

uint8_t TRX_getMode(void)
{
	return CurrentVFO()->Mode;
}

void TRX_RF_UNIT_UpdateState(bool clean) //передаём значения в RF-UNIT
{
	bool hpf_lock=false;
	HAL_GPIO_WritePin(RFUNIT_RCLK_GPIO_Port, RFUNIT_RCLK_Pin, GPIO_PIN_RESET); //защёлка
	for (uint8_t registerNumber = 0; registerNumber < 16; registerNumber++) {
		HAL_GPIO_WritePin(RFUNIT_CLK_GPIO_Port, RFUNIT_CLK_Pin, GPIO_PIN_RESET); //клок данных
		HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_RESET); //данные
		if(!clean)
		{
			if(registerNumber==0 && TRX_on_TX() && TRX_getMode() != TRX_MODE_LOOPBACK && TRX.TX_Amplifier) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //TX_AMP
			if(registerNumber==1 && TRX.ATT) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //ATT_ON
			if(registerNumber==2 && (!TRX.LPF || TRX_getFrequency()>LPF_END)) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //LPF_OFF
			if(registerNumber==3 && (!TRX.BPF || TRX_getFrequency()<BPF_1_START)) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_OFF
			if(registerNumber==4 && TRX.BPF && TRX_getFrequency()>=BPF_0_START && TRX_getFrequency()<BPF_0_END)
			{
				HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_0
				hpf_lock=true; //блокируем HPF для выделенного BPF фильтра УКВ
			}
			if(registerNumber==5 && TRX.BPF && TRX_getFrequency()>=BPF_1_START && TRX_getFrequency()<BPF_1_END) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_1
			if(registerNumber==6 && TRX.BPF && TRX_getFrequency()>=BPF_2_START && TRX_getFrequency()<BPF_2_END) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_2
			if(registerNumber==7 && TRX_on_TX() && TRX_getMode() != TRX_MODE_LOOPBACK) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //TX_RX
			
			//if(registerNumber==8) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); // unused
			//if(registerNumber==9) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); // unused
			//if(registerNumber==10 && TRX_on_TX() && TRX_getMode() != TRX_MODE_LOOPBACK) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //FAN
			if(registerNumber==11 && TRX.BPF && TRX_getFrequency()>=BPF_7_HPF && !hpf_lock) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_7_HPF
			if(registerNumber==12 && TRX.BPF && TRX_getFrequency()>=BPF_6_START && TRX_getFrequency()<BPF_6_END) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_6
			if(registerNumber==13 && TRX.BPF && TRX_getFrequency()>=BPF_5_START && TRX_getFrequency()<BPF_5_END) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_5
			if(registerNumber==14 && TRX.BPF && TRX_getFrequency()>=BPF_4_START && TRX_getFrequency()<BPF_4_END) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_4
			if(registerNumber==15 && TRX.BPF && TRX_getFrequency()>=BPF_3_START && TRX_getFrequency()<BPF_3_END) HAL_GPIO_WritePin(RFUNIT_DATA_GPIO_Port, RFUNIT_DATA_Pin, GPIO_PIN_SET); //BPF_3
		}
		HAL_GPIO_WritePin(RFUNIT_CLK_GPIO_Port, RFUNIT_CLK_Pin, GPIO_PIN_SET);
	}
	HAL_GPIO_WritePin(RFUNIT_CLK_GPIO_Port, RFUNIT_CLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RFUNIT_RCLK_GPIO_Port, RFUNIT_RCLK_Pin, GPIO_PIN_SET);
}

void TRX_DoAutoGain(void)
{
	//Process AutoGain feature
	if(TRX.AutoGain)
	{
		TRX.LPF=true;
		TRX.BPF=true;
		switch(autogain_stage)
		{
			case 0: //этап 1 - включаем ДПФ, ЛПФ, Аттенюатор, выключаем предусилитель (-12dB)
				TRX.Preamp=false;
				TRX.ATT=true;
				LCD_UpdateQuery.MainMenu=true;
				LCD_UpdateQuery.TopButtons=true;
				autogain_stage++;
				autogain_wait_reaction=0;
				//sendToDebug_str("AUTOGAIN LPF+BPF+ATT\r\n");
				break;
			case 1: //сменили состояние, обрабатываем результаты
				if((TRX_ADC_MAXAMPLITUDE*db2rateV(ATT_DB))<=AUTOGAIN_MAX_AMPLITUDE) //если можем выключить АТТ - переходим на следующий этап (+12dB)
					autogain_wait_reaction++; 
				else
					autogain_wait_reaction=0;
				if(autogain_wait_reaction>=AUTOGAIN_CORRECTOR_WAITSTEP)
				{
					autogain_stage++;
					autogain_wait_reaction=0;
				}
				break;
			case 2: //этап 1 - включаем ДПФ, ЛПФ, выключаем Аттенюатор, выключаем предусилитель (+0dB)
				TRX.Preamp=false;
				TRX.ATT=false;
				LCD_UpdateQuery.MainMenu=true;
				LCD_UpdateQuery.TopButtons=true;
				autogain_stage++;
				autogain_wait_reaction=0;
				//sendToDebug_str("AUTOGAIN LPF+BPF\r\n");
				break;
			case 3: //сменили состояние, обрабатываем результаты
				if(TRX_ADC_MAXAMPLITUDE>AUTOGAIN_MAX_AMPLITUDE) autogain_stage-=3; //слишком большое усиление, возвращаемся на этап назад
				if((TRX_ADC_MAXAMPLITUDE*db2rateV(PREAMP_GAIN_DB)/db2rateV(ATT_DB))<=AUTOGAIN_MAX_AMPLITUDE) //если можем включить АТТ+PREAMP - переходим на следующий этап (+20dB-12dB)
					autogain_wait_reaction++; 
				else
					autogain_wait_reaction=0;
				if(autogain_wait_reaction>=AUTOGAIN_CORRECTOR_WAITSTEP)
				{
					autogain_stage++;
					autogain_wait_reaction=0;
				}
				break;
			case 4: //этап 2 - включаем ДПФ, ЛПФ, Аттенюатор, Предусилитель (+8dB)
				TRX.Preamp=true;
				TRX.ATT=true;
				LCD_UpdateQuery.MainMenu=true;
				LCD_UpdateQuery.TopButtons=true;
				autogain_stage++;
				autogain_wait_reaction=0;
				//sendToDebug_str("AUTOGAIN LPF+BPF+PREAMP+ATT\r\n");
				break;
			case 5: //сменили состояние, обрабатываем результаты
				if(TRX_ADC_MAXAMPLITUDE>AUTOGAIN_MAX_AMPLITUDE) autogain_stage-=3; //слишком большое усиление, возвращаемся на этап назад
				if((TRX_ADC_MAXAMPLITUDE*db2rateV(ATT_DB))<=AUTOGAIN_MAX_AMPLITUDE) //если можем выключить АТТ - переходим на следующий этап (+12dB)
					autogain_wait_reaction++; 
				else
					autogain_wait_reaction=0;
				if(autogain_wait_reaction>=AUTOGAIN_CORRECTOR_WAITSTEP)
				{
					autogain_stage++;
					autogain_wait_reaction=0;
				}
				break;
			case 6: //этап 3 - включаем ДПФ, ЛПФ, Предусилитель, выключаем Аттенюатор (+20dB)
				TRX.Preamp=true;
				TRX.ATT=false;
				LCD_UpdateQuery.MainMenu=true;
				LCD_UpdateQuery.TopButtons=true;
				autogain_stage++;
				autogain_wait_reaction=0;
				//sendToDebug_str("AUTOGAIN LPF+BPF+PREAMP\r\n");
				break;
			case 7: //сменили состояние, обрабатываем результаты
				if(TRX_ADC_MAXAMPLITUDE>AUTOGAIN_MAX_AMPLITUDE) autogain_stage-=3; //слишком большое усиление, возвращаемся на этап назад
				break;
			default:
				autogain_stage=0;
				break;
		}
	}
}
