#include "lcd.h"
#include "settings.h"
#include "LCD/xpt2046_spi.h"

uint8_t systemMenuIndex=1;
const uint8_t systemMenuIndexCount=5;

void drawSystemMenu(void)
{
	if(LCD_busy)
	{
		LCD_UpdateQuery.SystemMenu=true;
		return;
	}
	if (!LCD_systemMenuOpened) return;
	if (LCD_timeMenuOpened) { LCD_Handler_SETTIME(); return; }
	LCD_busy=true;
	
	char ctmp[50];
	char y=5;
	char x1=5;
	char x2=250;
	char i=1;
	
	ILI9341_Fill(COLOR_BLACK);
	//
	ILI9341_printText("FFT Enabled", x1, y, COLOR_WHITE, COLOR_BLACK, 1);
	sprintf(ctmp, "%d", TRX.FFT_Enabled);
	ILI9341_printText(ctmp, x2, y, COLOR_WHITE, COLOR_BLACK, 1);
	if(systemMenuIndex==i) ILI9341_drawFastHLine(5,y+9,310,COLOR_WHITE);
	i++;
	y+=10;
	//
	ILI9341_printText("CW Generator shift, HZ", x1, y, COLOR_WHITE, COLOR_BLACK, 1);
	sprintf(ctmp, "%d", TRX.CW_GENERATOR_SHIFT_HZ);
	ILI9341_printText(ctmp, x2, y, COLOR_WHITE, COLOR_BLACK, 1);
	if(systemMenuIndex==i) ILI9341_drawFastHLine(5,y+9,310,COLOR_WHITE);
	i++;
	y+=10;
	//
	ILI9341_printText("Encoder slow rate", x1, y, COLOR_WHITE, COLOR_BLACK, 1);
	sprintf(ctmp, "%d", TRX.ENCODER_SLOW_RATE);
	ILI9341_printText(ctmp, x2, y, COLOR_WHITE, COLOR_BLACK, 1);
	if(systemMenuIndex==i) ILI9341_drawFastHLine(5,y+9,310,COLOR_WHITE);
	i++;
	y+=10;
	//
	//
	ILI9341_printText("LCD Calibrate", x1, y, COLOR_WHITE, COLOR_BLACK, 1);
	ILI9341_printText("RUN", x2, y, COLOR_WHITE, COLOR_BLACK, 1);
	if(systemMenuIndex==i) ILI9341_drawFastHLine(5,y+9,310,COLOR_WHITE);
	i++;
	y+=10;
	//
	ILI9341_printText("Set Clock Time", x1, y, COLOR_WHITE, COLOR_BLACK, 1);
	ILI9341_printText("RUN", x2, y, COLOR_WHITE, COLOR_BLACK, 1);
	if(systemMenuIndex==i) ILI9341_drawFastHLine(5,y+9,310,COLOR_WHITE);
	i++;
	y+=10;
	//
	ILI9341_Fill_RectXY(290,0,320,30,COLOR_GREEN);
	ILI9341_printText("X", 298, 5, COLOR_BLACK, COLOR_GREEN, 3);
	
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
		TRX.ENCODER_SLOW_RATE+=direction;
		if(TRX.ENCODER_SLOW_RATE<1) TRX.ENCODER_SLOW_RATE=1;
		if(TRX.ENCODER_SLOW_RATE>100) TRX.ENCODER_SLOW_RATE=100;
	}
	if(systemMenuIndex==4)
	{
		HAL_Delay(500);
		Touch_Calibrate();
		LCD_redraw();
	}
	if(systemMenuIndex==5) LCD_Handler_SETTIME();
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
