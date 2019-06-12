#include "system_menu.h"
#include "lcd.h"
#include "settings.h"
#include "audio_filters.h"
#include "bootloader.h"
#include "LCD/xpt2046_spi.h"

static void drawSystemMenuElement(char* title, SystemMenuType type, uint32_t *value, bool onlyVal);
static void redrawCurrentItem(void);
static void SYSMENU_HANDL_FFTEnabled(int8_t direction);
static void SYSMENU_HANDL_CW_GENERATOR_SHIFT_HZ(int8_t direction);
static void SYSMENU_HANDL_LCD_Brightness(int8_t direction);
static void SYSMENU_HANDL_ENCODER_SLOW_RATE(int8_t direction);
static void SYSMENU_HANDL_Touch_Calibrate(int8_t direction);
static void SYSMENU_HANDL_SETTIME(int8_t direction);
static void SYSMENU_HANDL_Standby_Time(int8_t direction);
static void SYSMENU_HANDL_Beeping(int8_t direction);
static void SYSMENU_HANDL_Key_timeout(int8_t direction);
static void SYSMENU_HANDL_FFT_Averaging(int8_t direction);
static void SYSMENU_HANDL_SSB_HPF_pass(int8_t direction);
static void SYSMENU_HANDL_Bootloader(int8_t direction);
static void SYSMENU_HANDL_CWDecoder(int8_t direction);

static struct sysmenu_item_handler sysmenu_handlers[] =
{
	{"SSB HPF Pass", SYSMENU_UINT16, (uint32_t *)&TRX.SSB_HPF_pass, SYSMENU_HANDL_SSB_HPF_pass},
	{"CW Key timeout", SYSMENU_UINT16, (uint32_t *)&TRX.Key_timeout, SYSMENU_HANDL_Key_timeout},
	{"CW Generator shift", SYSMENU_UINT16, (uint32_t *)&TRX.CW_GENERATOR_SHIFT_HZ, SYSMENU_HANDL_CW_GENERATOR_SHIFT_HZ},
	{"CW Decoder", SYSMENU_BOOLEAN, (uint32_t *)&TRX.CWDecoder, SYSMENU_HANDL_CWDecoder},
	{"Touchpad beeping", SYSMENU_BOOLEAN, (uint32_t *)&TRX.Beeping, SYSMENU_HANDL_Beeping},
	{"Set Clock Time", SYSMENU_RUN, 0, SYSMENU_HANDL_SETTIME},
	{"LCD Calibrate", SYSMENU_RUN, 0, SYSMENU_HANDL_Touch_Calibrate},
	{"LCD Brightness", SYSMENU_UINT8, (uint32_t *)&TRX.LCD_Brightness, SYSMENU_HANDL_LCD_Brightness},
	{"LCD Sleep Time", SYSMENU_UINT8, (uint32_t *)&TRX.Standby_Time, SYSMENU_HANDL_Standby_Time},
	{"Encoder slow rate", SYSMENU_UINT8, (uint32_t *)&TRX.ENCODER_SLOW_RATE, SYSMENU_HANDL_ENCODER_SLOW_RATE},
	{"FFT Enabled", SYSMENU_BOOLEAN, (uint32_t *)&TRX.FFT_Enabled, SYSMENU_HANDL_FFTEnabled},
	{"FFT Averaging", SYSMENU_UINT8, (uint32_t *)&TRX.FFT_Averaging, SYSMENU_HANDL_FFT_Averaging},
	{"Flash update", SYSMENU_RUN, 0, SYSMENU_HANDL_Bootloader},
};

static const uint8_t sysmenu_item_count = sizeof(sysmenu_handlers) / sizeof(sysmenu_handlers[0]);
static const uint8_t sysmenu_x1 = 5;
static const uint8_t sysmenu_x2 = 240;
static const uint16_t sysmenu_w = 288;
static uint8_t systemMenuIndex = 0;
static uint8_t sysmenu_y = 5;
static uint8_t sysmenu_i = 0;

void drawSystemMenu(bool draw_background)
{
	if (LCD_busy)
	{
		LCD_UpdateQuery.SystemMenu = true;
		return;
	}
	if (!LCD_systemMenuOpened) return;
	if (LCD_timeMenuOpened) { LCD_Handler_SETTIME(); return; }
	LCD_busy = true;

	sysmenu_i = 0;
	sysmenu_y = 5;

	if (draw_background) LCDDriver_Fill(COLOR_BLACK);

	for (uint8_t m = 0; m < sysmenu_item_count; m++)
		drawSystemMenuElement(sysmenu_handlers[m].title, sysmenu_handlers[m].type, sysmenu_handlers[m].value, false);

	LCDDriver_Fill_RectXY(290, 0, 320, 30, COLOR_GREEN);
	LCDDriver_printText("X", 298, 5, COLOR_BLACK, COLOR_GREEN, 3);

	LCDDriver_Fill_RectXY(290, 80, 320, 110, COLOR_GREEN);
	LCDDriver_printText("<", 298, 85, COLOR_BLACK, COLOR_GREEN, 3);

	LCDDriver_Fill_RectXY(290, 140, 320, 170, COLOR_GREEN);
	LCDDriver_printText(">", 298, 145, COLOR_BLACK, COLOR_GREEN, 3);

	LCD_UpdateQuery.SystemMenu = false;
	LCD_busy = false;
}

static void SYSMENU_HANDL_CWDecoder(int8_t direction)
{
	TRX.CWDecoder = !TRX.CWDecoder;
}

static void SYSMENU_HANDL_FFTEnabled(int8_t direction)
{
	TRX.FFT_Enabled = !TRX.FFT_Enabled;
}

static void SYSMENU_HANDL_CW_GENERATOR_SHIFT_HZ(int8_t direction)
{
	TRX.CW_GENERATOR_SHIFT_HZ += direction * 100;
	if (TRX.CW_GENERATOR_SHIFT_HZ < 100) TRX.CW_GENERATOR_SHIFT_HZ = 100;
	if (TRX.CW_GENERATOR_SHIFT_HZ > 10000) TRX.CW_GENERATOR_SHIFT_HZ = 10000;
}

static void SYSMENU_HANDL_LCD_Brightness(int8_t direction)
{
	TRX.LCD_Brightness += direction;
	if (TRX.LCD_Brightness < 1) TRX.LCD_Brightness = 1;
	if (TRX.LCD_Brightness > 100) TRX.LCD_Brightness = 100;
	LCDDriver_setBrightness(TRX.LCD_Brightness);
}

static void SYSMENU_HANDL_ENCODER_SLOW_RATE(int8_t direction)
{
	TRX.ENCODER_SLOW_RATE += direction;
	if (TRX.ENCODER_SLOW_RATE < 1) TRX.ENCODER_SLOW_RATE = 1;
	if (TRX.ENCODER_SLOW_RATE > 100) TRX.ENCODER_SLOW_RATE = 100;
}

static void SYSMENU_HANDL_Touch_Calibrate(int8_t direction)
{
	HAL_Delay(500);
	Touch_Calibrate();
	drawSystemMenu(true);
}

static void SYSMENU_HANDL_SETTIME(int8_t direction)
{
	LCD_Handler_SETTIME();
}

static void SYSMENU_HANDL_Standby_Time(int8_t direction)
{
	if (TRX.Standby_Time > 0 || direction > 0) TRX.Standby_Time += direction;
	if (TRX.Standby_Time > 250) TRX.Standby_Time = 250;
}

static void SYSMENU_HANDL_Beeping(int8_t direction)
{
	TRX.Beeping = !TRX.Beeping;
}

static void SYSMENU_HANDL_Key_timeout(int8_t direction)
{
	if (TRX.Key_timeout > 0 || direction > 0) TRX.Key_timeout += direction * 50;
	if (TRX.Key_timeout > 5000) TRX.Key_timeout = 5000;
}

static void SYSMENU_HANDL_FFT_Averaging(int8_t direction)
{
	TRX.FFT_Averaging += direction;
	if (TRX.FFT_Averaging < 1) TRX.FFT_Averaging = 1;
	if (TRX.FFT_Averaging > 10) TRX.FFT_Averaging = 10;
}

static void SYSMENU_HANDL_SSB_HPF_pass(int8_t direction)
{
	TRX.SSB_HPF_pass += direction * 100;
	if (TRX.SSB_HPF_pass < 100) TRX.SSB_HPF_pass = 100;
	if (TRX.SSB_HPF_pass > 500) TRX.SSB_HPF_pass = 500;
	ReinitAudioFilters();
}

static void SYSMENU_HANDL_Bootloader(int8_t direction)
{
	JumpToBootloader();
}

void eventRotateSystemMenu(int8_t direction)
{
	sysmenu_handlers[systemMenuIndex].menuHandler(direction);
	if (sysmenu_handlers[systemMenuIndex].type != SYSMENU_RUN)
		redrawCurrentItem();
}

void eventClickSystemMenu(uint16_t x, uint16_t y)
{
	if (x >= 290 && x <= 320 && y >= 1 && y <= 30)
	{
		LCD_systemMenuOpened = false;
		LCD_mainMenuOpened = true;
		LCD_UpdateQuery.Background = true;
		LCD_UpdateQuery.MainMenu = true;
		LCD_redraw();
		NeedSaveSettings = true;
	}
	else if (y < 120)
	{
		LCDDriver_drawFastHLine(0, (5 + systemMenuIndex * 18) + 17, sysmenu_w, COLOR_BLACK);
		if (systemMenuIndex > 0)
			systemMenuIndex--;
		redrawCurrentItem();
	}
	else
	{
		LCDDriver_drawFastHLine(0, (5 + systemMenuIndex * 18) + 17, sysmenu_w, COLOR_BLACK);
		if (systemMenuIndex < (sysmenu_item_count - 1))
			systemMenuIndex++;
		redrawCurrentItem();
	}
}

static void redrawCurrentItem(void)
{
	sysmenu_i = systemMenuIndex;
	sysmenu_y = 5 + systemMenuIndex * 18;
	drawSystemMenuElement(sysmenu_handlers[systemMenuIndex].title, sysmenu_handlers[systemMenuIndex].type, sysmenu_handlers[systemMenuIndex].value, true);
}

static void drawSystemMenuElement(char* title, SystemMenuType type, uint32_t *value, bool onlyVal)
{
	char ctmp[10];
	if (!onlyVal)
	{
		LCDDriver_Fill_RectXY(0, sysmenu_y, sysmenu_w, sysmenu_y + 17, COLOR_BLACK);
		LCDDriver_printText(title, sysmenu_x1, sysmenu_y, COLOR_WHITE, COLOR_BLACK, 2);
	}
	switch (type)
	{
	case SYSMENU_UINT8:
		sprintf(ctmp, "%d", (uint8_t)*value);
		break;
	case SYSMENU_UINT16:
		sprintf(ctmp, "%d", (uint16_t)*value);
		break;
	case SYSMENU_UINT32:
		sprintf(ctmp, "%d", (uint32_t)*value);
		break;
	case SYSMENU_INT8:
		sprintf(ctmp, "%d", (int8_t)*value);
		break;
	case SYSMENU_INT16:
		sprintf(ctmp, "%d", (int16_t)*value);
		break;
	case SYSMENU_INT32:
		sprintf(ctmp, "%d", (int32_t)*value);
		break;
	case SYSMENU_BOOLEAN:
		sprintf(ctmp, "%d", (int8_t)*value);
		if ((uint8_t)*value == 1)
			sprintf(ctmp, "YES");
		else
			sprintf(ctmp, "NO");
		break;
	case SYSMENU_RUN:
		sprintf(ctmp, "RUN");
		break;
	}
	if (onlyVal)
		LCDDriver_Fill_RectWH(sysmenu_x2, sysmenu_y, 4 * 12, 13, COLOR_BLACK);
	LCDDriver_printText(ctmp, sysmenu_x2, sysmenu_y, COLOR_WHITE, COLOR_BLACK, 2);
	if (systemMenuIndex == sysmenu_i) LCDDriver_drawFastHLine(0, sysmenu_y + 17, sysmenu_w, COLOR_WHITE);
	sysmenu_i++;
	sysmenu_y += 18;
}
