#ifndef LCD_h
#define LCD_h

#include "stm32f4xx_hal.h"
#include "trx_manager.h"
#include "LCD/MA_ILI9341.h"
#include "LCD/xpt2046_spi.h"

#define MENU_FREQ_HZ 1
#define MENU_FREQ_KHZ 2
#define MENU_FREQ_MHZ 3
#define MENU_FREQ_COUNT 3

#define MENU_MAIN_EXIT 1
#define MENU_MAIN_GAIN 2
#define MENU_MAIN_MICGAIN 3
#define MENU_MAIN_AGCSPEED 4
#define MENU_MAIN_COUNT 4

extern void LCD_Init(void);
void LCD_displayFreqInfo(bool force);
void LCD_displayTopButtons(bool redraw);
void LCD_checkTouchPad(void);
void LCD_displayMainMenu(void);
void LCD_displayStatusInfoBar(void);
void LCD_displayStatusInfoGUI(void);
extern void LCD_doEvents(void);
void LCD_redraw(void);
void LCD_resetTouchpadPins(void);

extern uint32_t LCD_last_showed_freq;
extern uint8_t LCD_menu_main_index;
extern bool LCD_mainMenuOpened;
extern bool LCD_needRedrawMainMenu;
extern char LCD_freq_string_hz[6];
extern char LCD_freq_string_khz[6];
extern char LCD_freq_string_mhz[6];
extern bool LCD_bandMenuOpened;
extern int LCD_last_s_meter;
extern bool LCD_busy;

#endif
