#include "main.h"
#include "lcd.h"
#include "functions.h"
#include "arm_math.h"
#include "agc.h"
#include "settings.h"
#include "system_menu.h"
#include "wm8731.h"
#include "audio_filters.h"
#include "LCD/fonts.h"
#include "LCD/xpt2046_spi.h"
#include "wm8731.h"
#include "usbd_ua3reo.h"
#include "noise_reduction.h"

char LCD_freq_string_hz[6];
char LCD_freq_string_khz[6];
char LCD_freq_string_mhz[6];
bool LCD_bandMenuOpened = false;
bool LCD_widthMenuOpened = false;
bool LCD_modeMenuOpened = false;
bool LCD_mainMenuOpened = false;
bool LCD_timeMenuOpened = false;
bool LCD_systemMenuOpened = false;
uint32_t LCD_last_showed_freq = 0;
uint16_t LCD_last_showed_freq_mhz = 9999;
uint16_t LCD_last_showed_freq_khz = 9999;
uint16_t LCD_last_showed_freq_hz = 9999;
uint8_t LCD_menu_main_index = MENU_MAIN_VOLUME;

int LCD_last_s_meter = 1;
bool LCD_busy = false;
bool LCD_pressed = false;

struct button_handler button_handlers[16];
uint8_t button_handlers_count = 0;
uint32_t lastTouchTick = 0;

uint32_t Time;
uint8_t Hours;
uint8_t Last_showed_Hours = 255;
uint8_t Minutes;
uint8_t Last_showed_Minutes = 255;
uint8_t Seconds;
uint8_t Last_showed_Seconds = 255;
uint8_t TimeMenuSelection = 0;

DEF_LCD_UpdateQuery LCD_UpdateQuery = { false,false,false,false,false,false,false };

void LCD_displayFreqInfo(void);
void LCD_displayTopButtons(bool redraw);
void LCD_displayMainMenu(void);
void LCD_displayStatusInfoBar(void);
void LCD_displayStatusInfoGUI(void);
void LCD_resetTouchpadPins(void);

void LCD_Init(void)
{
	LCDDriver_setBrightness(TRX.LCD_Brightness);
	LCDDriver_Init();
	LCDDriver_setRotation(4);
	LCDDriver_Fill(COLOR_WHITE);
	Init_XPT2046();
	LCD_redraw();
}

void LCD_displayTopButtons(bool redraw) { //вывод верхних кнопок
	if (LCD_mainMenuOpened) return;
	if (LCD_systemMenuOpened) return;
	if (LCD_busy)
	{
		LCD_UpdateQuery.TopButtons = true;
		return;
	}
	LCD_busy=true;
	if (redraw) LCDDriver_Fill_RectWH(0, 0, 319, 65, COLOR_BLACK);
	button_handlers_count = 0;

	//вывод диапазонов
	if (LCD_bandMenuOpened)
	{
		LCDDriver_Fill_RectWH(0, 0, 320, 130, COLOR_BLACK);
		int32_t freq_mhz = (int32_t)(TRX_getFrequency() / 1000000);

		printButton(5, 5, 58, 60, "1.8", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 1), LCD_Handler_BAND_160);
		printButton(68, 5, 58, 60, "3.5", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 3), LCD_Handler_BAND_80);
		printButton(131, 5, 58, 60, "7", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 7), LCD_Handler_BAND_40);
		printButton(194, 5, 58, 60, "10", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 10), LCD_Handler_BAND_30);
		printButton(257, 5, 58, 60, "BACK", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_MENU, false, LCD_Handler_BAND_BACK);
		printButton(5, 70, 58, 60, "14", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 14), LCD_Handler_BAND_20);
		printButton(68, 70, 58, 60, "18", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 18), LCD_Handler_BAND_17);
		printButton(131, 70, 58, 60, "21", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 21), LCD_Handler_BAND_15);
		printButton(194, 70, 58, 60, "24", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 24), LCD_Handler_BAND_12);
		printButton(257, 70, 58, 60, "28", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz == 28 && freq_mhz <= 29), LCD_Handler_BAND_10);
		printButton(5, 135, 58, 60, "FM1", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz >= 65 && freq_mhz < 74), LCD_Handler_BAND_FM1);
		printButton(68, 135, 58, 60, "FM2", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz >= 87 && freq_mhz < 108), LCD_Handler_BAND_FM2);
		printButton(131, 135, 58, 60, "VHF", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz >= 144 && freq_mhz < 146), LCD_Handler_BAND_VHF);
		printButton(194, 135, 58, 60, "UHF", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (freq_mhz >= 430 && freq_mhz < 440), LCD_Handler_BAND_UHF);
	}
	//вывод модов
	else if (LCD_modeMenuOpened)
	{
		LCDDriver_Fill(COLOR_BLACK);

		printButton(5, 5, 58, 60, MODE_DESCR[TRX_MODE_LSB], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_LSB), LCD_Handler_MODE_LSB);
		printButton(68, 5, 58, 60, MODE_DESCR[TRX_MODE_USB], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_USB), LCD_Handler_MODE_USB);
		printButton(131, 5, 58, 60, MODE_DESCR[TRX_MODE_IQ], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_IQ), LCD_Handler_MODE_IQ);
		printButton(194, 5, 58, 60, MODE_DESCR[TRX_MODE_AM], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_AM), LCD_Handler_MODE_AM);
		printButton(257, 5, 58, 60, "BACK", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_MENU, false, LCD_Handler_MODE_BACK);
		printButton(5, 70, 58, 60, MODE_DESCR[TRX_MODE_DIGI_L], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_DIGI_L), LCD_Handler_MODE_DIGL);
		printButton(68, 70, 58, 60, MODE_DESCR[TRX_MODE_DIGI_U], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_DIGI_U), LCD_Handler_MODE_DIGU);
		printButton(131, 70, 58, 60, MODE_DESCR[TRX_MODE_LOOPBACK], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_LOOPBACK), LCD_Handler_MODE_LOOP);
		printButton(194, 70, 58, 60, MODE_DESCR[TRX_MODE_NFM], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_NFM), LCD_Handler_MODE_NFM);
		printButton(257, 70, 58, 60, MODE_DESCR[TRX_MODE_WFM], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_WFM), LCD_Handler_MODE_WFM);
		printButton(5, 135, 58, 60, MODE_DESCR[TRX_MODE_CW_L], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_CW_L), LCD_Handler_MODE_CW_L);
		printButton(68, 135, 58, 60, MODE_DESCR[TRX_MODE_CW_U], COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_getMode() == TRX_MODE_CW_U), LCD_Handler_MODE_CW_U);
		printButton(131, 135, 58, 60, "", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, NULL);
		printButton(194, 135, 58, 60, "", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, NULL);
		printButton(257, 135, 58, 60, "", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, NULL);
	}
	//вывод аудио фильтров
	else if (LCD_widthMenuOpened)
	{
		LCDDriver_Fill_RectWH(0, 0, 320, 130, COLOR_BLACK);

		switch (TRX_getMode())
		{
		case TRX_MODE_CW_L:
		case TRX_MODE_CW_U:
			printButton(5, 5, 58, 60, "0.3", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 300), LCD_Handler_WIDTH_03);
			printButton(68, 5, 58, 60, "0.5", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 500), LCD_Handler_WIDTH_05);
			printButton(131, 5, 58, 60, "1.4", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 1400), LCD_Handler_WIDTH_14);
			printButton(194, 5, 58, 60, "1.6", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 1600), LCD_Handler_WIDTH_16);
			printButton(257, 5, 58, 60, "BACK", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_MENU, false, LCD_Handler_WIDTH_BACK);
			break;
		case TRX_MODE_NFM:
		case TRX_MODE_WFM:
			printButton(5, 5, 58, 60, "5", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 5000), LCD_Handler_WIDTH_50);
			printButton(68, 5, 58, 60, "6", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 6000), LCD_Handler_WIDTH_60);
			printButton(131, 5, 58, 60, "7", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 7000), LCD_Handler_WIDTH_70);
			printButton(194, 5, 58, 60, "8", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 8000), LCD_Handler_WIDTH_80);
			printButton(257, 5, 58, 60, "BACK", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_MENU, false, LCD_Handler_WIDTH_BACK);
			printButton(5, 70, 58, 60, "9", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 9000), LCD_Handler_WIDTH_90);
			printButton(68, 70, 58, 60, "9.5", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 9500), LCD_Handler_WIDTH_95);
			printButton(131, 70, 58, 60, "10", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 10000), LCD_Handler_WIDTH_100);
			printButton(194, 70, 58, 60, "15", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 15000), LCD_Handler_WIDTH_150);
			printButton(257, 70, 58, 60, "NONE", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 0), LCD_Handler_WIDTH_0);
			break;
		default:
			printButton(5, 5, 58, 60, "1.8", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 1800), LCD_Handler_WIDTH_18);
			printButton(68, 5, 58, 60, "2.1", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 2100), LCD_Handler_WIDTH_21);
			printButton(131, 5, 58, 60, "2.3", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 2300), LCD_Handler_WIDTH_23);
			printButton(194, 5, 58, 60, "2.5", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 2500), LCD_Handler_WIDTH_25);
			printButton(257, 5, 58, 60, "BACK", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, LCD_Handler_WIDTH_BACK);
			printButton(5, 70, 58, 60, "2.7", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 2700), LCD_Handler_WIDTH_27);
			printButton(68, 70, 58, 60, "2.9", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 2900), LCD_Handler_WIDTH_29);
			printButton(131, 70, 58, 60, "3.0", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 3000), LCD_Handler_WIDTH_30);
			printButton(194, 70, 58, 60, "3.2", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 3200), LCD_Handler_WIDTH_32);
			printButton(257, 70, 58, 60, "3.4", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (CurrentVFO()->Filter_Width == 3400), LCD_Handler_WIDTH_34);
			break;
		}
	}
	//вывод основных кнопок
	else
	{
		//верхний список
		printButton(0, 0, 76, 35, MODE_DESCR[TRX_getMode()], COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, LCD_Handler_MODE);
		printButton(81, 0, 76, 35, "WIDTH", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, LCD_Handler_WIDTH);
		printButton(162, 0, 76, 35, "BAND", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, LCD_Handler_BAND);
		printButton(243, 0, 76, 35, "MENU", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, false, LCD_Handler_MENU);
		printButton(0, 40, 60, 35, (!TRX.current_vfo) ? "VFOA" : "VFOB", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, TRX.current_vfo, LCD_Handler_VFO);
		printButton(65, 40, 60, 35, (!TRX.current_vfo) ? "B=A" : "A=B", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, !TRX.current_vfo, LCD_Handler_AB);
		printButton(130, 40, 60, 35, "FAST", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX.Fast == true), LCD_Handler_FAST);
		printButton(195, 40, 60, 35, "MUTE", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX.Mute == true), LCD_Handler_MUTE);
		printButton(260, 40, 59, 35, "TUNE", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, (TRX_tune == true), LCD_Handler_TUNE);
		//правый столбец
		printButton(265, 80, 54, 23, "PRE", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, TRX.Preamp, LCD_Handler_PREAMP);
		printButton(265, 108, 54, 23, "ATT", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, TRX.Att, LCD_Handler_ATT);
		printButton(265, 136, 54, 23, "AGC", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, TRX.Agc, LCD_Handler_AGC);
		printButton(265, 164, 54, 23, "NOT", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, TRX.Notch, LCD_Handler_NOTCH);
		printButton(265, 192, 54, 23, "DNR", COLOR_BUTTON_INACTIVE, COLOR_BUTTON_TEXT, COLOR_BUTTON_ACTIVE, TRX.DNR, LCD_Handler_DNR);
	}
	LCD_busy=false;
	LCD_UpdateQuery.TopButtons = false;
}

void LCD_displayFreqInfo() { //вывод частоты на экран
	if (LCD_mainMenuOpened) return;
	if (LCD_systemMenuOpened) return;
	if (LCD_bandMenuOpened) return;
	if (LCD_modeMenuOpened) return;
	if (LCD_widthMenuOpened) return;
	if (LCD_last_showed_freq == TRX_getFrequency()) return;
	if (LCD_busy)
	{
		LCD_UpdateQuery.FreqInfo = true;
		return;
	}
	LCD_busy = true;
	uint16_t mhz_x_offset=0;
	LCD_last_showed_freq = TRX_getFrequency();
	if (TRX_getFrequency() >= 100000000)
		mhz_x_offset=0;
	else if (TRX_getFrequency() >= 10000000)
		mhz_x_offset=26;
	else
		mhz_x_offset=52;
	LCDDriver_Fill_RectWH(0, 87, mhz_x_offset, 35, COLOR_BLACK);

	//добавляем пробелов для вывода частоты
	uint16_t hz=((uint32_t)TRX_getFrequency() % 1000);
	uint16_t khz=((uint32_t)(TRX_getFrequency() / 1000) % 1000);
	uint16_t mhz=((uint32_t)(TRX_getFrequency() / 1000000) % 1000000);
	sprintf(LCD_freq_string_hz, "%d", hz);
	sprintf(LCD_freq_string_khz, "%d", khz);
	sprintf(LCD_freq_string_mhz, "%d", mhz);
	
	if (LCD_last_showed_freq_mhz!=mhz)
	{
		if (TRX.LCD_menu_freq_index == MENU_FREQ_MHZ)
			LCDDriver_printTextFont(LCD_freq_string_mhz, mhz_x_offset, 120, COLOR_BLACK, COLOR_WHITE, FreeSans24pt7b);
		else
			LCDDriver_printTextFont(LCD_freq_string_mhz, mhz_x_offset, 120, COLOR_WHITE, COLOR_BLACK, FreeSans24pt7b);
		LCD_last_showed_freq_mhz=mhz;
	}
		
	char buff[50] = "";
	if (LCD_last_showed_freq_khz!=khz)
	{
		addSymbols(buff, LCD_freq_string_khz, 3, "0", false);
		if (TRX.LCD_menu_freq_index == MENU_FREQ_KHZ)
			LCDDriver_printTextFont(buff, 91, 120, COLOR_BLACK, COLOR_WHITE, FreeSans24pt7b);
		else
			LCDDriver_printTextFont(buff, 91, 120, COLOR_WHITE, COLOR_BLACK, FreeSans24pt7b);
		LCD_last_showed_freq_khz=khz;
	}
	if (LCD_last_showed_freq_hz!=hz)
	{
		addSymbols(buff, LCD_freq_string_hz, 3, "0", false);
		if (TRX.LCD_menu_freq_index == MENU_FREQ_HZ)
			LCDDriver_printTextFont(buff, 182, 120, COLOR_BLACK, COLOR_WHITE, FreeSans24pt7b);
		else
			LCDDriver_printTextFont(buff, 182, 120, COLOR_WHITE, COLOR_BLACK, FreeSans24pt7b);
		LCD_last_showed_freq_hz=hz;
	}
	NeedSaveSettings = true;
	LCD_UpdateQuery.FreqInfo = false;
	LCD_busy = false;
}

void LCD_displayStatusInfoGUI(void) { //вывод RX/TX и с-метра
	if (LCD_mainMenuOpened) return;
	if (LCD_systemMenuOpened) return;
	if (LCD_modeMenuOpened) return;
	if (LCD_bandMenuOpened) return;
	if (LCD_busy)
	{
		LCD_UpdateQuery.StatusInfoGUI = true;
		return;
	}
	LCD_busy = true;
	if (TRX_on_TX())
	{
		LCDDriver_Fill_RectWH(10,128,25,20,COLOR_BLACK);
		LCDDriver_printTextFont("TX", 10, 144, COLOR_RED, COLOR_BLACK, FreeSans9pt7b);
	}
	else
		LCDDriver_printTextFont("RX", 10, 144, COLOR_GREEN, COLOR_BLACK, FreeSans9pt7b);

	LCDDriver_drawRectXY(40, 130, 40 + METER_WIDTH, 145, COLOR_RED);

	LCDDriver_printTextFont(".", 78, 120, COLOR_WHITE, COLOR_BLACK, FreeSans24pt7b); //разделители частоты
	LCDDriver_printTextFont(".", 169, 120, COLOR_WHITE, COLOR_BLACK, FreeSans24pt7b);
	
	LCDDriver_printText("dBm",METER_WIDTH + 69,135,COLOR_GREEN,COLOR_BLACK,1);
	
	uint16_t step = METER_WIDTH / 8;
	LCDDriver_printText("1", 50 + step * 0, 150, COLOR_RED, COLOR_BLACK, 1);
	LCDDriver_printText("3", 50 + step * 1, 150, COLOR_RED, COLOR_BLACK, 1);
	LCDDriver_printText("5", 50 + step * 2, 150, COLOR_RED, COLOR_BLACK, 1);
	LCDDriver_printText("7", 50 + step * 3, 150, COLOR_RED, COLOR_BLACK, 1);
	LCDDriver_printText("9", 50 + step * 4, 150, COLOR_RED, COLOR_BLACK, 1);
	LCDDriver_printText("+20", 50 + step * 5, 150, COLOR_RED, COLOR_BLACK, 1);
	LCDDriver_printText("+40", 50 + step * 6, 150, COLOR_RED, COLOR_BLACK, 1);
	LCDDriver_printText("+60", 50 + step * 7, 150, COLOR_RED, COLOR_BLACK, 1);
	LCD_UpdateQuery.StatusInfoGUI = false;
	LCD_busy = false;
}

void LCD_displayStatusInfoBar(void) { //S-метра и прочей информации
	if (LCD_mainMenuOpened) return;
	if (LCD_systemMenuOpened) return;
	if (LCD_modeMenuOpened) return;
	if (LCD_bandMenuOpened) return;
	if (LCD_busy)
	{
		LCD_UpdateQuery.StatusInfoBar = true;
		return;
	}
	LCD_busy = true;
	char ctmp[50];
	const int width = METER_WIDTH-2;
	
	float32_t TRX_s_meter = (127+TRX_RX_dBm)/6; //127dbm - S0, 6dBm - 1S div
	if(TRX_s_meter<=9)
		TRX_s_meter=TRX_s_meter*((width/8)*5/9); //первые 9 баллов по 6 дб
	else
		TRX_s_meter=((width/8)*5)+(TRX_s_meter-9)*((width/8)*3/10); //остальные 3 балла по 10 дб
	if (TRX_s_meter > width) TRX_s_meter = width;
	if (TRX_s_meter < 0) TRX_s_meter = 0;

	int s_width = TRX_s_meter;
	if (LCD_last_s_meter > s_width) s_width = LCD_last_s_meter - ((LCD_last_s_meter - s_width) / 4); //сглаживаем движение с-метра
	else if (LCD_last_s_meter < s_width) s_width = s_width - ((s_width - LCD_last_s_meter) / 2);
	if (LCD_last_s_meter != s_width)
	{
		LCDDriver_Fill_RectWH(41 + s_width, 131, width - s_width, 13, COLOR_BLACK);
		LCDDriver_Fill_RectWH(41, 131, s_width, 13, COLOR_WHITE);
		LCD_last_s_meter = s_width;
	}
	
	sprintf(ctmp, "%d", TRX_RX_dBm);
	LCDDriver_Fill_RectWH(41 + width + 5,135,23,8,COLOR_BLACK);
	LCDDriver_printText(ctmp,41 + width + 5,135,COLOR_GREEN,COLOR_BLACK,1);
	
	LCDDriver_Fill_RectWH(270, 220, 50, 8, COLOR_BLACK);
	if (TRX_ADC_OTR || TRX_DAC_OTR) 
		LCDDriver_printText("OVR", 270, 220, COLOR_RED, COLOR_BLACK, 1);
	if (WM8731_Buffer_underrun && !TRX_on_TX()) 
		LCDDriver_printText("WBF", 297, 220, COLOR_RED, COLOR_BLACK, 1);
	if (FPGA_Buffer_underrun && TRX_on_TX()) 
		LCDDriver_printText("FBF", 297, 220, COLOR_RED, COLOR_BLACK, 1);
	if (false & RX_USB_AUDIO_underrun)
		LCDDriver_printText("UBF", 297, 220, COLOR_RED, COLOR_BLACK, 1);

	Time = RTC->TR;
	Hours = ((Time >> 20) & 0x03) * 10 + ((Time >> 16) & 0x0f);
	Minutes = ((Time >> 12) & 0x07) * 10 + ((Time >> 8) & 0x0f);
	Seconds = ((Time >> 4) & 0x07) * 10 + ((Time >> 0) & 0x0f);

	const uint8_t time_y=230;
	const uint16_t time_x=268;
	if (Hours != Last_showed_Hours)
	{
		sprintf(ctmp, "%d", Hours);
		addSymbols(ctmp, ctmp, 2, "0", false);
		LCDDriver_printText(ctmp, time_x, time_y, COLOR_WHITE, COLOR_BLACK, 1);
		LCDDriver_printText(":", time_x+12, time_y, COLOR_WHITE, COLOR_BLACK, 1);
		Last_showed_Hours = Hours;
	}
	if (Minutes != Last_showed_Minutes)
	{
		sprintf(ctmp, "%d", Minutes);
		addSymbols(ctmp, ctmp, 2, "0", false);
		LCDDriver_printText(ctmp, time_x+18, time_y, COLOR_WHITE, COLOR_BLACK, 1);
		LCDDriver_printText(":", time_x+30, time_y, COLOR_WHITE, COLOR_BLACK, 1);
		Last_showed_Minutes = Minutes;
	}
	if (Seconds != Last_showed_Seconds)
	{
		sprintf(ctmp, "%d", Seconds);
		addSymbols(ctmp, ctmp, 2, "0", false);
		LCDDriver_printText(ctmp, time_x+36, time_y, COLOR_WHITE, COLOR_BLACK, 1);
		Last_showed_Seconds = Seconds;
	}
	LCD_UpdateQuery.StatusInfoBar = false;
	LCD_busy = false;
}

void LCD_displayMainMenu() {
	if (!LCD_mainMenuOpened) return;
	if (LCD_busy)
	{
		LCD_UpdateQuery.MainMenu = true;
		return;
	}
	LCD_busy = true;
	button_handlers_count = 0;
	char ctmp[50];
	if (LCD_timeMenuOpened) { LCD_Handler_SETTIME(); return; }

	sprintf(ctmp, "%d", TRX.Volume);
	printMenuButton(5, 5, 74, 50, "VOLUME", ctmp, (LCD_menu_main_index == MENU_MAIN_VOLUME), false, LCD_Handler_MENU_VOLUME);
	sprintf(ctmp, "%d %%", TRX.RF_Gain);
	printMenuButton(84, 5, 74, 50, "RF GAIN", ctmp, (LCD_menu_main_index == MENU_MAIN_RF_GAIN), false, LCD_Handler_MENU_RF_GAIN);
	sprintf(ctmp, "%d %%", TRX.RF_Power);
	printMenuButton(163, 5, 74, 50, "POWER", ctmp, (LCD_menu_main_index == MENU_MAIN_RF_POWER), false, LCD_Handler_MENU_RF_POWER);
	printMenuButton(242, 5, 74, 50, "BACK", "to TRX", false, true, LCD_Handler_MENU_BACK);

	sprintf(ctmp, "%d", TRX.Agc_speed);
	printMenuButton(5, 60, 74, 50, "AGCSP", ctmp, (LCD_menu_main_index == MENU_MAIN_AGCSPEED), false, LCD_Handler_MENU_AGC_S);
	sprintf(ctmp, "%d", TRX.FM_SQL_threshold);
	printMenuButton(84, 60, 74, 50, "FM SQL", ctmp, (LCD_menu_main_index == MENU_MAIN_FM_SQL), false, LCD_Handler_MENU_FM_SQL);
	printMenuButton(163, 60, 74, 50, "MAP", "OF BANDS", TRX.BandMapEnabled, true, LCD_Handler_MENU_MAP);

	if(TRX.InputType==0) printMenuButton(242, 60, 74, 50, "INPUT", "Mic", true, true, LCD_Handler_MENU_LINEMIC);
	if(TRX.InputType==1) printMenuButton(242, 60, 74, 50, "INPUT", "Line", true, true, LCD_Handler_MENU_LINEMIC);
	if(TRX.InputType==2) printMenuButton(242, 60, 74, 50, "INPUT", "USB", true, true, LCD_Handler_MENU_LINEMIC);
	
	printMenuButton(242, 170, 74, 50, "SYSTEM", "MENU", false, true, LCD_Handler_MENU_SYSTEM_MENU);
	LCD_UpdateQuery.MainMenu = false;
	LCD_busy = false;
}

void LCD_redraw(void) {
	LCD_UpdateQuery.Background = true;
	LCD_UpdateQuery.FreqInfo = true;
	LCD_UpdateQuery.MainMenu = true;
	LCD_UpdateQuery.StatusInfoBar = true;
	LCD_UpdateQuery.StatusInfoGUI = true;
	LCD_UpdateQuery.TopButtons = true;
	LCD_UpdateQuery.SystemMenu = true;
	button_handlers_count = 0;
	LCD_last_s_meter = 0;
	LCD_last_showed_freq = 0;
	Last_showed_Hours = 255;
	Last_showed_Minutes = 255;
	Last_showed_Seconds = 255;
	LCD_last_showed_freq_mhz = 9999;
	LCD_last_showed_freq_khz = 9999;
	LCD_last_showed_freq_hz = 9999;
	LCD_doEvents();
}

void LCD_doEvents(void)
{
	if (LCD_busy) return;
	if(TRX_Time_InActive>TRX.Standby_Time && TRX.Standby_Time>0)
		LCDDriver_setBrightness(5);
	else
		LCDDriver_setBrightness(TRX.LCD_Brightness);
	
	if (LCD_UpdateQuery.Background)
	{
		LCD_busy = true;
		LCDDriver_Fill(COLOR_BLACK);
		LCD_UpdateQuery.Background = false;
		LCD_busy = false;
	}
	if (LCD_UpdateQuery.TopButtons) LCD_displayTopButtons(false);
	if (LCD_UpdateQuery.FreqInfo) LCD_displayFreqInfo();
	if (LCD_UpdateQuery.StatusInfoGUI) LCD_displayStatusInfoGUI();
	LCD_displayStatusInfoBar();
	if (LCD_UpdateQuery.MainMenu) LCD_displayMainMenu();
	if (LCD_UpdateQuery.SystemMenu) drawSystemMenu(false);
}

void LCD_Handler_TUNE(void)
{
	TRX_tune = !TRX_tune;
	TRX_ptt_hard = TRX_tune;
	LCD_displayStatusInfoGUI();
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
	TRX_Restart_Mode();
}

void LCD_Handler_MODE(void)
{
	LCD_modeMenuOpened = true;
	LCD_UpdateQuery.TopButtons = true;
}

void LCD_Handler_BAND(void)
{
	LCD_bandMenuOpened = true;
	LCD_redraw();
}

void LCD_Handler_WIDTH(void)
{
	LCD_widthMenuOpened = true;
	LCD_UpdateQuery.TopButtons = true;
}

void LCD_Handler_WIDTH_BACK(void)
{
	LCD_widthMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_WIDTH_03(void)
{
	CurrentVFO()->Filter_Width = 300;
	TRX.CW_Filter = 300;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_05(void)
{
	CurrentVFO()->Filter_Width = 500;
	TRX.CW_Filter = 500;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_14(void)
{
	CurrentVFO()->Filter_Width = 1400;
	TRX.SSB_Filter = 1400;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_16(void)
{
	CurrentVFO()->Filter_Width = 1600;
	TRX.SSB_Filter = 1600;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_18(void)
{
	CurrentVFO()->Filter_Width = 1800;
	TRX.SSB_Filter = 1800;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_21(void)
{
	CurrentVFO()->Filter_Width = 2100;
	TRX.SSB_Filter = 2100;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_23(void)
{
	CurrentVFO()->Filter_Width = 2300;
	TRX.SSB_Filter = 2300;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_25(void)
{
	CurrentVFO()->Filter_Width = 2500;
	TRX.SSB_Filter = 2500;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_27(void)
{
	CurrentVFO()->Filter_Width = 2700;
	TRX.SSB_Filter = 2700;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_29(void)
{
	CurrentVFO()->Filter_Width = 2900;
	TRX.SSB_Filter = 2900;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_30(void)
{
	CurrentVFO()->Filter_Width = 3000;
	TRX.SSB_Filter = 3000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_32(void)
{
	CurrentVFO()->Filter_Width = 3200;
	TRX.SSB_Filter = 3200;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_34(void)
{
	CurrentVFO()->Filter_Width = 3400;
	TRX.SSB_Filter = 3400;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_50(void)
{
	CurrentVFO()->Filter_Width = 5000;
	TRX.FM_Filter = 5000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_60(void)
{
	CurrentVFO()->Filter_Width = 6000;
	TRX.FM_Filter = 6000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_70(void)
{
	CurrentVFO()->Filter_Width = 7000;
	TRX.FM_Filter = 7000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_80(void)
{
	CurrentVFO()->Filter_Width = 8000;
	TRX.FM_Filter = 8000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_0(void)
{
	CurrentVFO()->Filter_Width = 0;
	TRX.FM_Filter = 0;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_85(void)
{
	CurrentVFO()->Filter_Width = 8500;
	TRX.FM_Filter = 8500;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_90(void)
{
	CurrentVFO()->Filter_Width = 9000;
	TRX.FM_Filter = 9000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_95(void)
{
	CurrentVFO()->Filter_Width = 9500;
	TRX.FM_Filter = 9500;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_100(void)
{
	CurrentVFO()->Filter_Width = 10000;
	TRX.FM_Filter = 10000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}
void LCD_Handler_WIDTH_150(void)
{
	CurrentVFO()->Filter_Width = 15000;
	TRX.FM_Filter = 15000;
	NeedSaveSettings = true;
	LCD_widthMenuOpened = false;
	ReinitAudioLPFFilter();
	LCD_redraw();
}

void LCD_Handler_PREAMP(void)
{
	TRX.Preamp = !TRX.Preamp;
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}

void LCD_Handler_AB(void)
{
	if(TRX.current_vfo)
	{
		TRX.VFO_A.Filter_Width=TRX.VFO_B.Filter_Width;
		TRX.VFO_A.Freq=TRX.VFO_B.Freq;
		TRX.VFO_A.Mode=TRX.VFO_B.Mode;
	}
	else
	{
		TRX.VFO_B.Filter_Width=TRX.VFO_A.Filter_Width;
		TRX.VFO_B.Freq=TRX.VFO_A.Freq;
		TRX.VFO_B.Mode=TRX.VFO_A.Mode;
	}
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}
void LCD_Handler_ATT(void)
{
	TRX.Att = !TRX.Att;
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}
void LCD_Handler_NOTCH(void)
{
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}
void LCD_Handler_DNR(void)
{
	TRX.DNR=!TRX.DNR;
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}

void LCD_Handler_AGC(void)
{
	TRX.Agc = !TRX.Agc;
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}

void LCD_Handler_VFO(void)
{
	TRX.current_vfo = !TRX.current_vfo;
	NeedSaveSettings = true;
	ReinitAudioLPFFilter();
	LCD_redraw();
}

void LCD_Handler_MUTE(void)
{
	TRX.Mute = !TRX.Mute;
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}

void LCD_Handler_FAST(void)
{
	TRX.Fast = !TRX.Fast;
	LCD_UpdateQuery.TopButtons = true;
	NeedSaveSettings = true;
}

void LCD_Handler_MENU_MAP(void)
{
	TRX.BandMapEnabled = !TRX.BandMapEnabled;
	LCD_UpdateQuery.MainMenu = true;
	NeedSaveSettings = true;
}

void LCD_Handler_MENU_SYSTEM_MENU(void)
{
	LCD_systemMenuOpened=true;
	LCD_UpdateQuery.Background=true;
	LCD_UpdateQuery.SystemMenu=true;
	LCD_doEvents();
}

void LCD_Handler_MENU_LINEMIC(void)
{
	TRX.InputType++;
	if(TRX.InputType==3) TRX.InputType=0;
	TRX_Restart_Mode();
	LCD_UpdateQuery.MainMenu = true;
	NeedSaveSettings = true;
}

void LCD_Handler_MENU(void)
{
	LCD_mainMenuOpened = true;
	LCD_redraw();
}

void LCD_Handler_BAND_160(void)
{
	TRX_setFrequency(TRX.saved_freq[0]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_80(void)
{
	TRX_setFrequency(TRX.saved_freq[1]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_40(void)
{
	TRX_setFrequency(TRX.saved_freq[2]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_30(void)
{
	TRX_setFrequency(TRX.saved_freq[3]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_BACK(void)
{
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_20(void)
{
	TRX_setFrequency(TRX.saved_freq[4]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_17(void)
{
	TRX_setFrequency(TRX.saved_freq[5]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_15(void)
{
	TRX_setFrequency(TRX.saved_freq[6]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_12(void)
{
	TRX_setFrequency(TRX.saved_freq[7]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_10(void)
{
	TRX_setFrequency(TRX.saved_freq[8]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_FM1(void)
{
	TRX_setFrequency(TRX.saved_freq[9]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_FM2(void)
{
	TRX_setFrequency(TRX.saved_freq[10]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_VHF(void)
{
	TRX_setFrequency(TRX.saved_freq[11]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_UHF(void)
{
	TRX_setFrequency(TRX.saved_freq[12]);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_LSB(void)
{
	TRX_setMode(TRX_MODE_LSB);
	CurrentVFO()->Filter_Width = TRX.SSB_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_USB(void)
{
	TRX_setMode(TRX_MODE_USB);
	CurrentVFO()->Filter_Width = TRX.SSB_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_IQ(void)
{
	TRX_setMode(TRX_MODE_IQ);
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_CW_L(void)
{
	TRX_setMode(TRX_MODE_CW_L);
	CurrentVFO()->Filter_Width = TRX.CW_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_CW_U(void)
{
	TRX_setMode(TRX_MODE_CW_U);
	CurrentVFO()->Filter_Width = TRX.CW_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_BACK(void)
{
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_DIGL(void)
{
	TRX_setMode(TRX_MODE_DIGI_L);
	CurrentVFO()->Filter_Width = TRX.SSB_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_DIGU(void)
{
	TRX_setMode(TRX_MODE_DIGI_U);
	CurrentVFO()->Filter_Width = TRX.SSB_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_NFM(void)
{
	TRX_setMode(TRX_MODE_NFM);
	CurrentVFO()->Filter_Width = TRX.FM_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_WFM(void)
{
	TRX_setMode(TRX_MODE_WFM);
	CurrentVFO()->Filter_Width = 0;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_AM(void)
{
	TRX_setMode(TRX_MODE_AM);
	CurrentVFO()->Filter_Width = TRX.SSB_Filter;
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_LOOP(void)
{
	TRX_setMode(TRX_MODE_LOOPBACK);
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MENU_BACK(void)
{
	LCD_mainMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MENU_VOLUME(void)
{
	LCD_menu_main_index = MENU_MAIN_VOLUME;
	LCD_UpdateQuery.MainMenu = true;
}

void LCD_Handler_MENU_RF_GAIN(void)
{
	LCD_menu_main_index = MENU_MAIN_RF_GAIN;
	LCD_UpdateQuery.MainMenu = true;
}

void LCD_Handler_MENU_FM_SQL(void)
{
	LCD_menu_main_index = MENU_MAIN_FM_SQL;
	LCD_UpdateQuery.MainMenu = true;
}

void LCD_Handler_MENU_RF_POWER(void)
{
	LCD_menu_main_index = MENU_MAIN_RF_POWER;
	LCD_UpdateQuery.MainMenu = true;
}

void LCD_Handler_MENU_AGC_S(void)
{
	LCD_menu_main_index = MENU_MAIN_AGCSPEED;
	LCD_UpdateQuery.MainMenu = true;
}

void LCD_Handler_SETTIME(void)
{
	if (!LCD_timeMenuOpened) LCDDriver_Fill(COLOR_BLACK);
	LCD_timeMenuOpened = true;
	button_handlers_count = 0;
	char ctmp[50];
	Time = RTC->TR;
	Hours = ((Time >> 20) & 0x03) * 10 + ((Time >> 16) & 0x0f);
	Minutes = ((Time >> 12) & 0x07) * 10 + ((Time >> 8) & 0x0f);
	Seconds = ((Time >> 4) & 0x07) * 10 + ((Time >> 0) & 0x0f);
	sprintf(ctmp, "%d", Hours);
	addSymbols(ctmp, ctmp, 2, "0", false);
	LCDDriver_printText(ctmp, 76, 100, COLOR_BUTTON_TEXT, TimeMenuSelection == 0 ? COLOR_WHITE : COLOR_BLACK, 3);
	LCDDriver_printText(":", 124, 100, COLOR_BUTTON_TEXT, COLOR_BLACK, 3);
	sprintf(ctmp, "%d", Minutes);
	addSymbols(ctmp, ctmp, 2, "0", false);
	LCDDriver_printText(ctmp, 148, 100, COLOR_BUTTON_TEXT, TimeMenuSelection == 1 ? COLOR_WHITE : COLOR_BLACK, 3);
	LCDDriver_printText(":", 194, 100, COLOR_BUTTON_TEXT, COLOR_BLACK, 3);
	sprintf(ctmp, "%d", Seconds);
	addSymbols(ctmp, ctmp, 2, "0", false);
	LCDDriver_printText(ctmp, 220, 100, COLOR_BUTTON_TEXT, TimeMenuSelection == 2 ? COLOR_WHITE : COLOR_BLACK, 3);
	printButton(50, 170, 76, 30, ">", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_MENU, false, LCD_Handler_TIMEMENU_NEXT);
	printButton(200, 170, 76, 30, "BACK", COLOR_BUTTON_MENU, COLOR_BUTTON_TEXT, COLOR_BUTTON_MENU, false, LCD_Handler_TIMEMENU_BACK);
}

void LCD_Handler_TIMEMENU_NEXT(void)
{
	LCDDriver_Fill(COLOR_BLACK);
	TimeMenuSelection++;
	if (TimeMenuSelection == 3) TimeMenuSelection = 0;
	LCD_UpdateQuery.SystemMenu = true;
}

void LCD_Handler_TIMEMENU_BACK(void)
{
	LCDDriver_Fill(COLOR_BLACK);
	LCD_timeMenuOpened = false;
	LCD_UpdateQuery.SystemMenu = true;
}

void LCD_checkTouchPad(void)
{
	if (!isTouch())
	{
		LCD_pressed = false;
		return;
	}
	if (LCD_pressed) return;
	LCD_pressed = true;

	if (HAL_GetTick() - lastTouchTick < TOUCHPAD_DELAY) return; //anti-bounce
	lastTouchTick = HAL_GetTick();

	uint16_t x = 0;
	uint16_t y = 0;
	Get_Touch_XY(&x, &y, 1, 0);
	char dest[100];
	sprintf(dest, "Touchpad x = %d  y = %d\r\n", x, y);
	sendToDebug_str(dest);
	WM8731_Beep();

	if(LCD_systemMenuOpened && !LCD_timeMenuOpened)
	{
		eventClickSystemMenu(x, y);
		return;
	}
	
	for (uint8_t i = 0; i < button_handlers_count; i++)
		if (button_handlers[i].x1 <= x && button_handlers[i].x2 >= x && button_handlers[i].y1 <= y && button_handlers[i].y2 >= y && button_handlers[i].handler != 0)
		{
			button_handlers[i].handler();
			return;
		}

	if (!LCD_bandMenuOpened && !LCD_mainMenuOpened && !LCD_modeMenuOpened && !LCD_widthMenuOpened && !LCD_systemMenuOpened)
	{
		if (x >= 5 && x <= 80 && y >= 80 && y <= 121) TRX.LCD_menu_freq_index = MENU_FREQ_MHZ;
		if (x >= 95 && x <= 170 && y >= 80 && y <= 121) TRX.LCD_menu_freq_index = MENU_FREQ_KHZ;
		if (x >= 195 && x <= 270 && y >= 80 && y <= 121) TRX.LCD_menu_freq_index = MENU_FREQ_HZ;
		LCD_last_showed_freq = 0;
		LCD_last_showed_freq_mhz = 9999;
		LCD_last_showed_freq_khz = 9999;
		LCD_last_showed_freq_hz = 9999;
		LCD_displayFreqInfo();
	}
}

void printButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* text, uint16_t back_color, uint16_t text_color, uint16_t active_color, bool active, void(*onclick) ())
{
	LCDDriver_Fill_RectWH(x, y, width, height, active ? active_color : back_color);
	uint16_t x1, y1, w, h;
	LCDDriver_getTextBounds(text, x, y, &x1, &y1, &w, &h, FreeSans9pt7b);
	LCDDriver_printTextFont(text, x + (width - w) / 2, y + (height / 2) + h / 2, text_color, active ? active_color : back_color, FreeSans9pt7b);
	button_handlers[button_handlers_count].x1 = x;
	button_handlers[button_handlers_count].x2 = x + width;
	button_handlers[button_handlers_count].y1 = y;
	button_handlers[button_handlers_count].y2 = y + height;
	button_handlers[button_handlers_count].handler = onclick;
	button_handlers_count++;
}

void printMenuButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* text1, char* text2, bool active, bool switchable, void(*onclick) ())
{
	uint16_t color = active ? COLOR_BUTTON_MENU : COLOR_BUTTON_ACTIVE;
	if (!switchable) color = active ? COLOR_BUTTON_MENU : COLOR_BUTTON_INACTIVE;
	LCDDriver_Fill_RectWH(x, y, width, height, color);

	uint16_t x1, y1, w, h;
	LCDDriver_getTextBounds(text1, x, y, &x1, &y1, &w, &h, FreeSans9pt7b);
	LCDDriver_printTextFont(text1, x + (width - w) / 2, y + (h) / 2 + h + 2, COLOR_BUTTON_TEXT, color, FreeSans9pt7b);

	LCDDriver_printText(text2, x + (width - strlen(text2) * 6 * 1) / 2 + 1, y + (height - 8 * 2 - 8 * 1) / 2 + 8 * 2 + 4, COLOR_BLACK, color, 1);
	button_handlers[button_handlers_count].x1 = x;
	button_handlers[button_handlers_count].x2 = x + width;
	button_handlers[button_handlers_count].y1 = y;
	button_handlers[button_handlers_count].y2 = y + height;
	button_handlers[button_handlers_count].handler = onclick;
	button_handlers_count++;
}

void LCD_showError(char text[])
{
	LCD_busy=true;
	LCDDriver_Fill(COLOR_RED);
	LCDDriver_printTextFont(text,5,110,COLOR_WHITE,COLOR_RED,FreeSans12pt7b);
	HAL_IWDG_Refresh(&hiwdg);
	HAL_Delay(3000);
	HAL_IWDG_Refresh(&hiwdg);
	LCD_busy=false;
	LCD_redraw();
}
