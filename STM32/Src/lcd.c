#include "lcd.h"
#include "functions.h"
#include "arm_math.h"
#include "agc.h"
#include "settings.h"
#include "wm8731.h"

char LCD_freq_string_hz[6];
char LCD_freq_string_khz[6];
char LCD_freq_string_mhz[6];
bool LCD_bandMenuOpened = false;
bool LCD_modeMenuOpened = false;
bool LCD_mainMenuOpened = false;
uint32_t LCD_last_showed_freq = 0;
uint8_t LCD_menu_main_index = 1;
bool LCD_needRedrawMainMenu = false;
int LCD_last_s_meter = 1;
bool LCD_busy = false;
bool LCD_pressed = false;

void LCD_Init(void)
{
	HAL_GPIO_WritePin(LED_BL_GPIO_Port, LED_BL_Pin, GPIO_PIN_RESET); //turn on LED BL
	ILI9341_Init();
	ILI9341_setRotation(4);
	ILI9341_Fill(COLOR_WHITE);
	Init_XPT2046();
	LCD_redraw();
}

void LCD_displayTopButtons(bool redraw) { //вывод верхних кнопок
	if (LCD_mainMenuOpened) return;
	if (redraw) ILI9341_Fill_RectWH(0, 0, 319, 65, COLOR_BLACK);
	int color = COLOR_CYAN;

	//вывод основных кнопок
	if (!LCD_bandMenuOpened)
	{
		color = COLOR_DGREEN;
		ILI9341_Fill_RectWH(5, 5, 73, 30, color);
		if(TRX_getMode()==TRX_MODE_LSB) ILI9341_printText("LSB", 24, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_USB) ILI9341_printText("USB", 24, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_IQ) ILI9341_printText("IQ", 29, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_CW) ILI9341_printText("CW", 29, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_DIGI_L) ILI9341_printText("DIGL", 18, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_DIGI_U) ILI9341_printText("DIGU", 18, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_NO_TX) ILI9341_printText("NOTX", 24, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_FM) ILI9341_printText("FM", 29, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_AM) ILI9341_printText("AM", 29, 14, COLOR_BLUE, color, 2);
		if(TRX_getMode()==TRX_MODE_LOOPBACK) ILI9341_printText("LOOP", 18, 14, COLOR_BLUE, color, 2);

		color = COLOR_CYAN;
		//if (TRX_getMode() == TRX_MODE_IQ) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(83, 5, 73, 30, color);
		ILI9341_printText("VFO-A", 90, 14, COLOR_BLUE, color, 2);

		color = COLOR_CYAN;
		if (TRX_tune) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(161, 5, 73, 30, color);
		ILI9341_printText("TUNE", 174, 14, COLOR_BLUE, color, 2);

		ILI9341_Fill_RectWH(239, 5, 76, 30, COLOR_DGREEN);
		ILI9341_printText("BAND", 255, 14, COLOR_BLUE, COLOR_DGREEN, 2);

		//

		color = COLOR_CYAN;
		if (TRX.Preamp) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(5, 40, 53, 30, color);
		ILI9341_printText("PRE", 14, 50, COLOR_BLUE, color, 2);

		color = COLOR_CYAN;
		if (TRX.Agc) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(63, 40, 53, 30, color);
		ILI9341_printText("AGC", 73, 50, COLOR_BLUE, color, 2);

		color = COLOR_CYAN;
		if (TRX.BandMapEnabled) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(121, 40, 53, 30, color);
		ILI9341_printText("MAP", 133, 50, COLOR_BLUE, color, 2);

		color = COLOR_CYAN;
		//if (TRX_tune) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(178, 40, 56, 30, color);
		ILI9341_printText("ATT", 189, 50, COLOR_BLUE, color, 2);

		ILI9341_Fill_RectWH(239, 40, 76, 30, COLOR_DGREEN);
		ILI9341_printText("MENU", 255, 50, COLOR_BLUE, COLOR_DGREEN, 2);
	}

	//вывод диапазонов
	if (LCD_bandMenuOpened)
	{
		ILI9341_Fill_RectWH(0, 0, 320, 130, COLOR_BLACK);
		int32_t freq_mhz = (int32_t)(TRX_getFrequency() / 1000000);
		color = COLOR_CYAN; if (freq_mhz == 1) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(5, 5, 58, 60, color); //1.8
		ILI9341_printText("1.8", 16, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (freq_mhz == 3) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(68, 5, 58, 60, color); //3.5
		ILI9341_printText("3.5", 79, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (freq_mhz == 7) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(131, 5, 58, 60, color); //7
		ILI9341_printText("7", 155, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (freq_mhz == 10) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(194, 5, 58, 60, color); //10
		ILI9341_printText("10", 210, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN;
		ILI9341_Fill_RectWH(257, 5, 58, 60, color); //BACK
		ILI9341_printText("BACK", 262, 30, COLOR_BLUE, color, 2);

		color = COLOR_CYAN; if (freq_mhz == 14) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(5, 70, 58, 60, color); //14
		ILI9341_printText("14", 21, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (freq_mhz == 18) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(68, 70, 58, 60, color); //18
		ILI9341_printText("18", 86, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (freq_mhz == 21) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(131, 70, 58, 60, color); //21
		ILI9341_printText("21", 148, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (freq_mhz == 24) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(194, 70, 58, 60, color); //24
		ILI9341_printText("24", 211, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (freq_mhz == 28) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(257, 70, 58, 60, color); //28
		ILI9341_printText("28", 274, 91, COLOR_BLUE, color, 2);
	}
	
	//вывод модов
	if (LCD_modeMenuOpened)
	{
		ILI9341_Fill_RectWH(0, 0, 320, 130, COLOR_BLACK);
		
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_LSB) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(5, 5, 58, 60, color); //1.8
		ILI9341_printText("LSB", 16, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_USB) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(68, 5, 58, 60, color); //3.5
		ILI9341_printText("USB", 79, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_IQ) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(131, 5, 58, 60, color); //7
		ILI9341_printText("IQ", 145, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_CW) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(194, 5, 58, 60, color); //10
		ILI9341_printText("CW", 211, 30, COLOR_BLUE, color, 2);
		color = COLOR_CYAN;
		ILI9341_Fill_RectWH(257, 5, 58, 60, color); //BACK
		ILI9341_printText("BACK", 262, 30, COLOR_BLUE, color, 2);

		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_DIGI_L) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(5, 70, 58, 60, color); //14
		ILI9341_printText("DIGL", 11, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_DIGI_U) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(68, 70, 58, 60, color); //18
		ILI9341_printText("DIGU", 76, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_FM) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(131, 70, 58, 60, color); //21
		ILI9341_printText("FM", 148, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_AM) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(194, 70, 58, 60, color); //24
		ILI9341_printText("AM", 211, 91, COLOR_BLUE, color, 2);
		color = COLOR_CYAN; if (TRX_getMode()==TRX_MODE_LOOPBACK) color = COLOR_YELLOW;
		ILI9341_Fill_RectWH(257, 70, 58, 60, color); //28
		ILI9341_printText("LOOP", 264, 91, COLOR_BLUE, color, 2);
	}
}

void LCD_displayFreqInfo(bool force) { //вывод частоты на экран
	if (LCD_mainMenuOpened) return;
	if (LCD_bandMenuOpened) return;
	if (LCD_modeMenuOpened) return;
	if (LCD_last_showed_freq == TRX_getFrequency()) return;
	if(LCD_busy && !force) return;
	LCD_busy=true;
	LCD_last_showed_freq = TRX_getFrequency();
	//закрашиваем лишнее на экране если разрядов не хватает
	if (TRX_getFrequency() < 10000000)
	{
		ILI9341_Fill_RectWH(0, 80, 30, 41, COLOR_BLACK);
		ILI9341_Fill_RectWH(301, 80, 19, 41, COLOR_BLACK);
	}

	if (TRX_getFrequency() > 10000000)
		ILI9341_SetCursorPosition(10, 80);
	else
		ILI9341_SetCursorPosition(30, 80);

	//добавляем пробелов для вывода частоты
	sprintf(LCD_freq_string_hz, "%d", ((uint32_t)TRX_getFrequency() % 1000));
	sprintf(LCD_freq_string_khz, "%d", ((uint32_t)(TRX_getFrequency() / 1000) % 1000));
	sprintf(LCD_freq_string_mhz, "%d", ((uint32_t)(TRX_getFrequency() / 1000000) % 1000000));

	if (TRX.LCD_menu_freq_index == MENU_FREQ_MHZ) ILI9341_printText(LCD_freq_string_mhz, ILI9341_GetCurrentXOffset(), 80, COLOR_BLACK, COLOR_WHITE, 5);
	else ILI9341_printText(LCD_freq_string_mhz, ILI9341_GetCurrentXOffset(), 80, COLOR_WHITE, COLOR_BLACK, 5);

	ILI9341_printText(".", ILI9341_GetCurrentXOffset(), 80, COLOR_WHITE, COLOR_BLACK, 5);

	char buff[50] = "";
	addSymbols(buff, LCD_freq_string_khz, 3, "0", false);
	if (TRX.LCD_menu_freq_index == MENU_FREQ_KHZ) ILI9341_printText(buff, ILI9341_GetCurrentXOffset(), 80, COLOR_BLACK, COLOR_WHITE, 5);
	else ILI9341_printText(buff, ILI9341_GetCurrentXOffset(), 80, COLOR_WHITE, COLOR_BLACK, 5);

	ILI9341_printText(".", ILI9341_GetCurrentXOffset(), 80, COLOR_WHITE, COLOR_BLACK, 5);

	addSymbols(buff, LCD_freq_string_hz, 3, "0", false);
	if (TRX.LCD_menu_freq_index == MENU_FREQ_HZ) ILI9341_printText(buff, ILI9341_GetCurrentXOffset(), 80, COLOR_BLACK, COLOR_WHITE, 5);
	else ILI9341_printText(buff, ILI9341_GetCurrentXOffset(), 80, COLOR_WHITE, COLOR_BLACK, 5);

	NeedSaveSettings=true;
	LCD_busy=false;
}

void LCD_displayStatusInfoGUI(void) { //вывод RX/TX и с-метра
	if (LCD_mainMenuOpened) return;
	if (TRX_ptt)
		ILI9341_printText("TX", 10, 130, COLOR_RED, COLOR_BLACK, 2);
	else
		ILI9341_printText("RX", 10, 130, COLOR_GREEN, COLOR_BLACK, 2);

	int width = 275;
	ILI9341_drawRectXY(40, 130, 40 + width, 145, COLOR_RED);

	int step = width / 8;
	ILI9341_printText("1", 50 + step * 0, 150, COLOR_RED, COLOR_BLACK, 1);
	ILI9341_printText("3", 50 + step * 1, 150, COLOR_RED, COLOR_BLACK, 1);
	ILI9341_printText("5", 50 + step * 2, 150, COLOR_RED, COLOR_BLACK, 1);
	ILI9341_printText("7", 50 + step * 3, 150, COLOR_RED, COLOR_BLACK, 1);
	ILI9341_printText("9", 50 + step * 4, 150, COLOR_RED, COLOR_BLACK, 1);
	ILI9341_printText("+20", 50 + step * 5, 150, COLOR_RED, COLOR_BLACK, 1);
	ILI9341_printText("+40", 50 + step * 6, 150, COLOR_RED, COLOR_BLACK, 1);
	ILI9341_printText("+60", 50 + step * 7, 150, COLOR_RED, COLOR_BLACK, 1);
}

void LCD_displayStatusInfoBar(void) { //S-метра и прочей информации
	if (LCD_mainMenuOpened) return;

	int width = 273;
	TRX_s_meter = (float32_t)72.25091874*log10f_fast(agc_wdsp.volts) + ((float32_t)0.35*(float32_t)72.25091874);
	if (TRX_s_meter > width) TRX_s_meter = width;
	if (TRX_s_meter < 0) TRX_s_meter = 0;

	int s_width = TRX_s_meter;
	if (LCD_last_s_meter > s_width) s_width = LCD_last_s_meter - ((LCD_last_s_meter - s_width) / 6); //сглаживаем движение с-метра
	if (LCD_last_s_meter < s_width) s_width = s_width - ((s_width - LCD_last_s_meter) / 2);
	ILI9341_Fill_RectWH(41 + s_width, 131, width - s_width, 13, COLOR_BLACK);
	LCD_last_s_meter = s_width;
	ILI9341_Fill_RectWH(41, 131, s_width, 13, COLOR_WHITE);

	ILI9341_Fill_RectWH(300, 210, 30, 30, COLOR_BLACK);
	if (TRX_agc_wdsp_action && TRX.Agc && (TRX.Mode == TRX_MODE_LSB || TRX.Mode == TRX_MODE_USB)) ILI9341_printText("AGC", 300, 210, COLOR_GREEN, COLOR_BLACK, 1);
	if (TRX_ADC_OTR) ILI9341_printText("OVR", 300, 220, COLOR_RED, COLOR_BLACK, 1);
	if (WM8731_Buffer_underrun) ILI9341_printText("BUF", 300, 230, COLOR_RED, COLOR_BLACK, 1);
	
}

void LCD_displayMainMenu() {
	if (!LCD_mainMenuOpened) return;

	char ctmp[50];
	char buff[50] = "";
	int y = 5;

	ILI9341_printText("Exit", 5, y, COLOR_WHITE, COLOR_BLACK, 2);
	if (LCD_menu_main_index == MENU_MAIN_EXIT) ILI9341_printText("->", 200, y, COLOR_BLACK, COLOR_WHITE, 2);
	else ILI9341_printText("->", 200, y, COLOR_WHITE, COLOR_BLACK, 2);
	y += 20;

	ILI9341_printText("Digital Gain:", 5, y, COLOR_WHITE, COLOR_BLACK, 2);
	sprintf(ctmp, "%d", TRX.Gain_level);
	addSymbols(buff, ctmp, 2, " ", true);
	if (LCD_menu_main_index == MENU_MAIN_GAIN) ILI9341_printText(buff, 200, y, COLOR_BLACK, COLOR_WHITE, 2);
	else ILI9341_printText(buff, 200, y, COLOR_WHITE, COLOR_BLACK, 2);
	y += 20;

	ILI9341_printText("MIC Gain:", 5, y, COLOR_WHITE, COLOR_BLACK, 2);
	sprintf(ctmp, "%d", TRX.MicGain_level);
	addSymbols(buff, ctmp, 2, " ", true);
	if (LCD_menu_main_index == MENU_MAIN_MICGAIN) ILI9341_printText(buff, 200, y, COLOR_BLACK, COLOR_WHITE, 2);
	else ILI9341_printText(buff, 200, y, COLOR_WHITE, COLOR_BLACK, 2);
	y += 20;
	
	ILI9341_printText("AGC Speed:", 5, y, COLOR_WHITE, COLOR_BLACK, 2);
	sprintf(ctmp, "%d", TRX.Agc_speed);
	addSymbols(buff, ctmp, 2, " ", true);
	if (LCD_menu_main_index == MENU_MAIN_AGCSPEED) ILI9341_printText(buff, 200, y, COLOR_BLACK, COLOR_WHITE, 2);
	else ILI9341_printText(buff, 200, y, COLOR_WHITE, COLOR_BLACK, 2);
	y += 20;
	
	ILI9341_Fill_RectWH(100, 200, 50, 30, COLOR_YELLOW);
	ILI9341_printText("v", 120, 210, COLOR_BLUE, COLOR_YELLOW, 2);
	ILI9341_Fill_RectWH(160, 200, 50, 30, COLOR_YELLOW);
	ILI9341_printText("^", 180, 210, COLOR_BLUE, COLOR_YELLOW, 2);
}

void LCD_redraw(void) {
	ILI9341_Fill_RectWH(0, 0, 319, 239, COLOR_BLACK);
	LCD_displayTopButtons(false);
	LCD_last_showed_freq = 0;
	LCD_displayFreqInfo(false);
	LCD_displayStatusInfoGUI();
	LCD_displayStatusInfoBar();
	LCD_displayMainMenu();
}

void LCD_doEvents(void)
{
	if(LCD_busy) return;
	LCD_busy=true;
	LCD_displayFreqInfo(true);
	LCD_displayStatusInfoBar();
	if (LCD_needRedrawMainMenu) {
		LCD_needRedrawMainMenu = false;
		LCD_displayMainMenu();
	}
	LCD_busy=false;
}

void LCD_checkTouchPad(void)
{
	if (!isTouch())
	{
		LCD_pressed=false;
		return;
	}
	if(LCD_pressed) return;
	LCD_pressed=true;
	uint16_t x = 0;
	uint16_t y = 0;
	Get_Touch_XY(&x, &y, 1, 0);
	char dest[100];
	sprintf(dest, "Touchpad x = %d  y = %d\r\n", x, y);
	logToUART1_str(dest);

	if (!LCD_bandMenuOpened && !LCD_mainMenuOpened && !LCD_modeMenuOpened)
	{
		if (x >= 5 && x <= 78 && y >= 5 && y <= 35) { LCD_modeMenuOpened=true; LCD_displayTopButtons(true); } //кнопка Mode
		//if (x >= 83 && x <= 156 && y >= 5 && y <= 35) { TRX_setMode(TRX_MODE_USB); LCD_displayTopButtons(false); } //кнопка VFO
		if (x >= 161 && x <= 234 && y >= 5 && y <= 35) { //кнопка TUNE
			TRX_tune = !TRX_tune; 
			TRX_ptt = TRX_tune;
			LCD_displayStatusInfoGUI();
			LCD_displayTopButtons(false);
			NeedSaveSettings=true;
			start_i2s();
		} //кнопка TUNE
		if (x >= 239 && x <= 315 && y >= 5 && y <= 35) {
			LCD_bandMenuOpened = true;  //кнопка BAND
			LCD_displayTopButtons(true);
		}
		
		if (x >= 5 && x <= 58 && y >= 40 && y <= 70) {
			TRX.Preamp = !TRX.Preamp; //кнопка PREAMP
			LCD_displayTopButtons(false);
			NeedSaveSettings=true;
		}
		else if (x >= 63 && x <= 116 && y >= 40 && y <= 70) {
			TRX.Agc = !TRX.Agc; //кнопка AGC
			LCD_displayTopButtons(false);
			NeedSaveSettings=true;
		}
		else if (x >= 121 && x <= 174 && y >= 40 && y <= 70) { //кнопка BandMap
			TRX.BandMapEnabled=!TRX.BandMapEnabled;
			LCD_displayTopButtons(false);
			NeedSaveSettings=true;
		}
		else if (x >= 178 && x <= 234 && y >= 40 && y <= 70) { //кнопка ATT
			
		}
		else if (x >= 239 && x <= 315 && y >= 40 && y <= 70) {
			LCD_mainMenuOpened = true; //кнопка MENU
			LCD_redraw();
		}
		
		if(x>=30 && x<=60 && y>=80 && y<=121) TRX.LCD_menu_freq_index=MENU_FREQ_MHZ;
		if(x>=90 && x<=180 && y>=80 && y<=121) TRX.LCD_menu_freq_index=MENU_FREQ_KHZ;
		if(x>=210 && x<=300 && y>=80 && y<=121) TRX.LCD_menu_freq_index=MENU_FREQ_HZ;
		LCD_last_showed_freq=0;
		LCD_displayFreqInfo(false);
	}
	else if (LCD_bandMenuOpened && !LCD_mainMenuOpened && !LCD_modeMenuOpened)
	{
		if (x >= 1 && x <= 63 && y >= 1 && y <= 65) {
			TRX_setFrequency(1850000);  //кнопка 1.8
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 68 && x <= 126 && y >= 1 && y <= 65) {
			TRX_setFrequency(3600000);  //кнопка 3.5
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 131 && x <= 189 && y >= 1 && y <= 65) {
			TRX_setFrequency(7100000);  //кнопка 7
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 194 && x <= 252 && y >= 1 && y <= 65) {
			TRX_setFrequency(10130000);  //кнопка 10
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 257 && x <= 319 && y >= 1 && y <= 65) { //кнопка BACK
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 1 && x <= 63 && y >= 68 && y <= 130) {
			TRX_setFrequency(14100000);  //кнопка 14
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 68 && x <= 126 && y >= 68 && y <= 130) {
			TRX_setFrequency(18100000);  //кнопка 18
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 131 && x <= 189 && y >= 68 && y <= 130) {
			TRX_setFrequency(21100000);  //кнопка 21
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 194 && x <= 252 && y >= 68 && y <= 130) {
			TRX_setFrequency(24900000);  //кнопка 24
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 257 && x <= 319 && y >= 68 && y <= 130) {
			TRX_setFrequency(28100000);  //кнопка 28
			LCD_bandMenuOpened = false;
			LCD_redraw();
		}
	}
	else if (!LCD_bandMenuOpened && !LCD_mainMenuOpened && LCD_modeMenuOpened)
	{
		if (x >= 1 && x <= 63 && y >= 1 && y <= 65) {
			TRX_setMode(TRX_MODE_LSB);  //кнопка LSB
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 68 && x <= 126 && y >= 1 && y <= 65) {
			TRX_setMode(TRX_MODE_USB);  //кнопка USB
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 131 && x <= 189 && y >= 1 && y <= 65) {
			TRX_setMode(TRX_MODE_IQ);  //кнопка IQ
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 194 && x <= 252 && y >= 1 && y <= 65) {
			TRX_setMode(TRX_MODE_CW);  //кнопка CW
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 257 && x <= 319 && y >= 1 && y <= 65) { //кнопка BACK
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 1 && x <= 63 && y >= 68 && y <= 130) {
			TRX_setMode(TRX_MODE_DIGI_L);  //кнопка DIGL
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 68 && x <= 126 && y >= 68 && y <= 130) {
			TRX_setMode(TRX_MODE_DIGI_U);  //кнопка DIGU
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 131 && x <= 189 && y >= 68 && y <= 130) {
			TRX_setMode(TRX_MODE_FM);  //кнопка FM
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 194 && x <= 252 && y >= 68 && y <= 130) {
			TRX_setMode(TRX_MODE_AM);  //кнопка AM
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
		if (x >= 257 && x <= 319 && y >= 68 && y <= 130) {
			TRX_setMode(TRX_MODE_LOOPBACK);  //кнопка LOOP
			LCD_modeMenuOpened = false;
			LCD_redraw();
		}
	}
	
	//кнопки в меню
	if (LCD_mainMenuOpened)
	{
		if (x >= 100 && x <= 150 && y >= 200 && y <= 230) { //кнопка v
			LCD_menu_main_index++;
		}
		else if (x >= 160 && x <= 210 && y >= 200 && y <= 230) { //кнопка ^
			LCD_menu_main_index--;
		}
		if (LCD_menu_main_index > MENU_MAIN_COUNT) LCD_menu_main_index = 1;
		if (LCD_menu_main_index < 1) LCD_menu_main_index = 1;
		LCD_needRedrawMainMenu=true;
	}
}

