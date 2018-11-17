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

struct button_handler button_handlers[16];
uint8_t button_handlers_count=0;
uint32_t lastTouchTick=0;

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
	button_handlers_count=0;

		//вывод диапазонов
	if (LCD_bandMenuOpened)
	{
		ILI9341_Fill_RectWH(0, 0, 320, 130, COLOR_BLACK);
		int32_t freq_mhz = (int32_t)(TRX_getFrequency() / 1000000);
		
		printButton(5, 5, 58, 60, "1.8", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 1), LCD_Handler_BAND_160);
		printButton(68, 5, 58, 60, "3.5", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 3), LCD_Handler_BAND_80);
		printButton(131, 5, 58, 60, "7", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 7), LCD_Handler_BAND_40);
		printButton(194, 5, 58, 60, "10", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 10), LCD_Handler_BAND_30);
		printButton(257, 5, 58, 60, "BACK", COLOR_DGREEN, COLOR_BLUE, COLOR_DGREEN, false, LCD_Handler_BAND_BACK);
		printButton(5, 70, 58, 60, "14", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 14), LCD_Handler_BAND_20);
		printButton(68, 70, 58, 60, "18", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 18), LCD_Handler_BAND_17);
		printButton(131, 70, 58, 60, "21", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 21), LCD_Handler_BAND_15);
		printButton(194, 70, 58, 60, "24", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 24), LCD_Handler_BAND_12);
		printButton(257, 70, 58, 60, "28", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (freq_mhz == 28), LCD_Handler_BAND_10);
	}
	//вывод модов
	else if (LCD_modeMenuOpened)
	{
		ILI9341_Fill_RectWH(0, 0, 320, 130, COLOR_BLACK);
		
		printButton(5, 5, 58, 60, MODE_DESCR[TRX_MODE_LSB], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_LSB), LCD_Handler_MODE_LSB);
		printButton(68, 5, 58, 60, MODE_DESCR[TRX_MODE_USB], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_USB), LCD_Handler_MODE_USB);
		printButton(131, 5, 58, 60, MODE_DESCR[TRX_MODE_IQ], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_IQ), LCD_Handler_MODE_IQ);
		printButton(194, 5, 58, 60, MODE_DESCR[TRX_MODE_CW], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_CW), LCD_Handler_MODE_CW);
		printButton(257, 5, 58, 60, "BACK", COLOR_DGREEN, COLOR_BLUE, COLOR_DGREEN, false, LCD_Handler_MODE_BACK);
		printButton(5, 70, 58, 60, MODE_DESCR[TRX_MODE_DIGI_L], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_DIGI_L), LCD_Handler_MODE_DIGL);
		printButton(68, 70, 58, 60, MODE_DESCR[TRX_MODE_DIGI_U], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_DIGI_U), LCD_Handler_MODE_DIGU);
		printButton(131, 70, 58, 60, MODE_DESCR[TRX_MODE_FM], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_FM), LCD_Handler_MODE_FM);
		printButton(194, 70, 58, 60, MODE_DESCR[TRX_MODE_AM], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_AM), LCD_Handler_MODE_AM);
		printButton(257, 70, 58, 60, MODE_DESCR[TRX_MODE_LOOPBACK], COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_getMode()==TRX_MODE_LOOPBACK), LCD_Handler_MODE_LOOP);
	}
	//вывод основных кнопок
	else
	{
		printButton(5, 5, 73, 30, MODE_DESCR[TRX_getMode()], COLOR_DGREEN, COLOR_BLUE, COLOR_DGREEN, false, LCD_Handler_MODE);
		printButton(83, 5, 73, 30, "VFO-A", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, false, 0);
		printButton(161, 5, 73, 30, "TUNE", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX_tune==true), LCD_Handler_TUNE);
		printButton(239, 5, 76, 30, "BAND", COLOR_DGREEN, COLOR_BLUE, COLOR_DGREEN, false, LCD_Handler_BAND);
		printButton(5, 40, 53, 30, "PRE", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX.Preamp==true), LCD_Handler_PREAMP);
		printButton(63, 40, 53, 30, "AGC", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX.Agc==true), LCD_Handler_AGC);
		printButton(121, 40, 53, 30, "MAP", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, (TRX.BandMapEnabled==true), LCD_Handler_MAP);
		printButton(179, 40, 55, 30, "ATT", COLOR_CYAN, COLOR_BLUE, COLOR_YELLOW, false, 0);
		printButton(239, 40, 76, 30, "MENU", COLOR_DGREEN, COLOR_BLUE, COLOR_DGREEN, false, LCD_Handler_MENU);
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
	TRX_s_meter = (float32_t)73*log10f_fast(agc_wdsp.volts*32767) + ((float32_t)0.35*(float32_t)73);
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
	button_handlers_count=0;
	char ctmp[50];
	
	printMenuButton(5, 5, 70, 50, "BACK", "to TRX", false, LCD_Handler_MENU_BACK);
	sprintf(ctmp, "%d", TRX.Gain_level);
	printMenuButton(80, 5, 70, 50, "GAIN", ctmp, (LCD_menu_main_index == MENU_MAIN_GAIN), LCD_Handler_MENU_GAIN);
	sprintf(ctmp, "%d", TRX.MicGain_level);
	printMenuButton(155, 5, 70, 50, "MIC", ctmp, (LCD_menu_main_index == MENU_MAIN_MICGAIN), LCD_Handler_MENU_MIC_G);
	sprintf(ctmp, "%d", TRX.Agc_speed);
	printMenuButton(230, 5, 70, 50, "AGCSP", ctmp, (LCD_menu_main_index == MENU_MAIN_AGCSPEED), LCD_Handler_MENU_AGC_S);
	printMenuButton(5, 60, 70, 50, "LCD", "CALIBRATE", false, LCD_Handler_LCD_Calibrate);
}

void LCD_redraw(void) {
	ILI9341_Fill_RectWH(0, 0, 319, 239, COLOR_BLACK);
	button_handlers_count=0;
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

void LCD_Handler_TUNE(void)
{
	TRX_tune = !TRX_tune; 
	TRX_ptt = TRX_tune;
	LCD_displayStatusInfoGUI();
	LCD_displayTopButtons(false);
	NeedSaveSettings=true;
	start_i2s();
}

void LCD_Handler_MODE(void)
{
	LCD_modeMenuOpened=true;
	LCD_displayTopButtons(true);
}

void LCD_Handler_BAND(void)
{
	LCD_bandMenuOpened = true;
	LCD_displayTopButtons(true);
}

void LCD_Handler_PREAMP(void)
{
	TRX.Preamp = !TRX.Preamp;
	LCD_displayTopButtons(false);
	NeedSaveSettings=true;
}

void LCD_Handler_AGC(void)
{
	TRX.Agc = !TRX.Agc;
	LCD_displayTopButtons(false);
	NeedSaveSettings=true;
}

void LCD_Handler_MAP(void)
{
	TRX.BandMapEnabled=!TRX.BandMapEnabled;
	LCD_displayTopButtons(false);
	NeedSaveSettings=true;
}

void LCD_Handler_MENU(void)
{
	LCD_mainMenuOpened = true;
	LCD_redraw();
}

void LCD_Handler_BAND_160(void)
{
	TRX_setFrequency(1850000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_80(void)
{
	TRX_setFrequency(3600000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_40(void)
{
	TRX_setFrequency(7100000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_30(void)
{
	TRX_setFrequency(10130000);
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
	TRX_setFrequency(14100000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_17(void)
{
	TRX_setFrequency(18100000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_15(void)
{
	TRX_setFrequency(21100000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_12(void)
{
	TRX_setFrequency(24900000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_BAND_10(void)
{
	TRX_setFrequency(28100000);
	LCD_bandMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_LSB(void)
{
	TRX_setMode(TRX_MODE_LSB);
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_USB(void)
{
	TRX_setMode(TRX_MODE_USB);
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_IQ(void)
{
	TRX_setMode(TRX_MODE_IQ);
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_CW(void)
{
	TRX_setMode(TRX_MODE_CW);
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
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_DIGU(void)
{
	TRX_setMode(TRX_MODE_DIGI_U);
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_FM(void)
{
	TRX_setMode(TRX_MODE_FM);
	LCD_modeMenuOpened = false;
	LCD_redraw();
}

void LCD_Handler_MODE_AM(void)
{
	TRX_setMode(TRX_MODE_AM);
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

void LCD_Handler_MENU_GAIN(void)
{
	LCD_menu_main_index = MENU_MAIN_GAIN;
	LCD_needRedrawMainMenu = true;
}

void LCD_Handler_MENU_MIC_G(void)
{
	LCD_menu_main_index = MENU_MAIN_MICGAIN;
	LCD_needRedrawMainMenu = true;
}

void LCD_Handler_MENU_AGC_S(void)
{
	LCD_menu_main_index = MENU_MAIN_AGCSPEED;
	LCD_needRedrawMainMenu = true;
}

void LCD_Handler_LCD_Calibrate(void)
{
	HAL_Delay(500);
	Touch_Calibrate();
	LCD_redraw();
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
	
	if(HAL_GetTick()-lastTouchTick<TOUCHPAD_DELAY) return; //anti-bounce
	lastTouchTick=HAL_GetTick();
	
	uint16_t x = 0;
	uint16_t y = 0;
	Get_Touch_XY(&x, &y, 1, 0);
	char dest[100];
	sprintf(dest, "Touchpad x = %d  y = %d\r\n", x, y);
	logToUART1_str(dest);

	for(uint8_t i=0;i<button_handlers_count;i++)
		if(button_handlers[i].x1<=x && button_handlers[i].x2>=x && button_handlers[i].y1<=y && button_handlers[i].y2>=y && button_handlers[i].handler!=0)
		{
			button_handlers[i].handler();
			return;
		}
	
	if (!LCD_bandMenuOpened && !LCD_mainMenuOpened && !LCD_modeMenuOpened)
	{
		if(x>=30 && x<=60 && y>=80 && y<=121) TRX.LCD_menu_freq_index=MENU_FREQ_MHZ;
		if(x>=90 && x<=180 && y>=80 && y<=121) TRX.LCD_menu_freq_index=MENU_FREQ_KHZ;
		if(x>=210 && x<=300 && y>=80 && y<=121) TRX.LCD_menu_freq_index=MENU_FREQ_HZ;
		LCD_last_showed_freq=0;
		LCD_displayFreqInfo(false);
	}
}

void printButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* text, uint16_t back_color, uint16_t text_color, uint16_t active_color, bool active, void (*onclick) ())
{
	uint8_t font_size=2;
	ILI9341_Fill_RectWH(x, y, width, height, active ? active_color : back_color);
	ILI9341_printText(text, x+(width-strlen(text)*6*font_size)/2+2, y+(height-8*font_size)/2+2, text_color, active ? active_color : back_color, font_size);
	button_handlers[button_handlers_count].x1=x;
	button_handlers[button_handlers_count].x2=x+width;
	button_handlers[button_handlers_count].y1=y;
	button_handlers[button_handlers_count].y2=y+height;
	button_handlers[button_handlers_count].handler=onclick;
	button_handlers_count++;
}

void printMenuButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* text1, char* text2, bool active, void (*onclick) ())
{
	ILI9341_Fill_RectWH(x, y, width, height, active ? COLOR_YELLOW : COLOR_CYAN);
	ILI9341_printText(text1, x+(width-strlen(text1)*6*2)/2+2, y+(height-8*2-8*1)/2, COLOR_BLUE, active ? COLOR_YELLOW : COLOR_CYAN, 2);
	ILI9341_printText(text2, x+(width-strlen(text2)*6*1)/2+2, y+(height-8*2-8*1)/2+8*2+4, COLOR_BLACK, active ? COLOR_YELLOW : COLOR_CYAN, 1);
	button_handlers[button_handlers_count].x1=x;
	button_handlers[button_handlers_count].x2=x+width;
	button_handlers[button_handlers_count].y1=y;
	button_handlers[button_handlers_count].y2=y+height;
	button_handlers[button_handlers_count].handler=onclick;
	button_handlers_count++;
}
