#include "system_menu.h"
#include "lcd.h"
#include "settings.h"
#include "LCD/xpt2046_spi.h"

uint8_t systemMenuIndex=1;
uint8_t systemMenuIndexCount=0;

const uint8_t sysmenu_x1=5;
const uint8_t sysmenu_x2=240;
uint8_t sysmenu_y=5;
uint8_t sysmenu_i=1;

void drawSystemMenu(bool draw_background)
{
	if(LCD_busy)
	{
		LCD_UpdateQuery.SystemMenu=true;
		return;
	}
	if (!LCD_systemMenuOpened) return;
	if (LCD_timeMenuOpened) { LCD_Handler_SETTIME(); return; }
	LCD_busy=true;
	
	sysmenu_i=1;
	sysmenu_y=5;
	
	if(draw_background) LCDDriver_Fill(COLOR_BLACK);
	drawSystemMenuElement("FFT Enabled", SYSMENU_BOOLEAN, TRX.FFT_Enabled);
	drawSystemMenuElement("CW Generator shift", SYSMENU_INTEGER, TRX.CW_GENERATOR_SHIFT_HZ);
	drawSystemMenuElement("LCD Brightness", SYSMENU_INTEGER, TRX.LCD_Brightness);
	drawSystemMenuElement("Encoder slow rate", SYSMENU_INTEGER, TRX.ENCODER_SLOW_RATE);
	drawSystemMenuElement("LCD Calibrate", SYSMENU_RUN, 0);
	drawSystemMenuElement("Set Clock Time", SYSMENU_RUN, 0);
	drawSystemMenuElement("Time to standby", SYSMENU_INTEGER, TRX.Standby_Time);
	drawSystemMenuElement("Touchpad beeping", SYSMENU_BOOLEAN, TRX.Beeping);
	drawSystemMenuElement("CW Key timeout", SYSMENU_INTEGER, TRX.Key_timeout);
	drawSystemMenuElement("FFT Averaging", SYSMENU_INTEGER, TRX.FFT_Averaging);
	
	LCDDriver_Fill_RectXY(290,0,320,30,COLOR_GREEN);
	LCDDriver_printText("X", 298, 5, COLOR_BLACK, COLOR_GREEN, 3);
	
	LCDDriver_Fill_RectXY(290,80,320,110,COLOR_GREEN);
	LCDDriver_printText("<", 298, 85, COLOR_BLACK, COLOR_GREEN, 3);
	
	LCDDriver_Fill_RectXY(290,140,320,170,COLOR_GREEN);
	LCDDriver_printText(">", 298, 145, COLOR_BLACK, COLOR_GREEN, 3);
	
	LCD_UpdateQuery.SystemMenu=false;
	LCD_busy=false;
}

void eventRotateSystemMenu(int direction)
{
	if(systemMenuIndex==1) TRX.FFT_Enabled=!TRX.FFT_Enabled;
	if(systemMenuIndex==2)
	{
		TRX.CW_GENERATOR_SHIFT_HZ+=direction*100;
		if(TRX.CW_GENERATOR_SHIFT_HZ<100) TRX.CW_GENERATOR_SHIFT_HZ=100;
		if(TRX.CW_GENERATOR_SHIFT_HZ>10000) TRX.CW_GENERATOR_SHIFT_HZ=10000;
	}
	if(systemMenuIndex==3)
	{
		TRX.LCD_Brightness+=direction;
		if(TRX.LCD_Brightness<1) TRX.LCD_Brightness=1;
		if(TRX.LCD_Brightness>100) TRX.LCD_Brightness=100;
		LCDDriver_setBrightness(TRX.LCD_Brightness);
	}
	if(systemMenuIndex==4)
	{
		TRX.ENCODER_SLOW_RATE+=direction;
		if(TRX.ENCODER_SLOW_RATE<1) TRX.ENCODER_SLOW_RATE=1;
		if(TRX.ENCODER_SLOW_RATE>100) TRX.ENCODER_SLOW_RATE=100;
	}
	if(systemMenuIndex==5)
	{
		HAL_Delay(500);
		Touch_Calibrate();
		LCD_redraw();
	}
	if(systemMenuIndex==6) LCD_Handler_SETTIME();
	if(systemMenuIndex==7)
	{
		if(TRX.Standby_Time>0 || direction>0) TRX.Standby_Time+=direction;
		if(TRX.Standby_Time>250) TRX.Standby_Time=250;
	}
	if(systemMenuIndex==8) TRX.Beeping=!TRX.Beeping;
	if(systemMenuIndex==9)
	{
		if(TRX.Key_timeout>0 || direction>0) TRX.Key_timeout+=direction*50;
		if(TRX.Key_timeout>5000) TRX.Key_timeout=5000;
	}
	if(systemMenuIndex==10)
	{
		TRX.FFT_Averaging+=direction;
		if(TRX.FFT_Averaging<1) TRX.FFT_Averaging=1;
		if(TRX.FFT_Averaging>10) TRX.FFT_Averaging=10;
	}
	LCD_UpdateQuery.SystemMenu=true;
}

void eventClickSystemMenu(uint16_t x, uint16_t y)
{
	if(x>=290 && x<=320 && y>=1 && y<=30)
	{
		LCD_systemMenuOpened=false;
		LCD_mainMenuOpened=true;
		LCD_UpdateQuery.Background=true;
		LCD_UpdateQuery.MainMenu=true;
		LCD_redraw();
		NeedSaveSettings=true;
	}
	else if(y<120)
	{
		if(systemMenuIndex>1)
			systemMenuIndex--;
		LCD_UpdateQuery.SystemMenu=true;
	}
	else
	{
		if(systemMenuIndex<systemMenuIndexCount)
			systemMenuIndex++;
		LCD_UpdateQuery.SystemMenu=true;
	}
}

void drawSystemMenuElement(char* title, SystemMenuType type, uint32_t value)
{
	char ctmp[50];
	LCDDriver_Fill_RectXY(1,sysmenu_y,320,sysmenu_y+17,COLOR_BLACK);
	LCDDriver_printText(title, sysmenu_x1, sysmenu_y, COLOR_WHITE, COLOR_BLACK, 2);
	if(type==SYSMENU_INTEGER) sprintf(ctmp, "%d", value);
	if(type==SYSMENU_BOOLEAN && value==1) sprintf(ctmp, "YES");
	if(type==SYSMENU_BOOLEAN && value==0) sprintf(ctmp, "NO");
	if(type==SYSMENU_RUN) sprintf(ctmp, "RUN");
	LCDDriver_printText(ctmp, sysmenu_x2, sysmenu_y, COLOR_WHITE, COLOR_BLACK, 2);
	if(systemMenuIndex==sysmenu_i) LCDDriver_drawFastHLine(5,sysmenu_y+17,310,COLOR_WHITE);
	systemMenuIndexCount++;
	sysmenu_i++;
	sysmenu_y+=18;
}
