#ifndef LCD_h
#define LCD_h

#include "stm32f4xx_hal.h"
#include "trx_manager.h"
#include "LCD/lcd_driver.h"

#define MENU_FREQ_HZ 1
#define MENU_FREQ_KHZ 2
#define MENU_FREQ_MHZ 3
#define MENU_FREQ_COUNT 3

#define MENU_MAIN_EXIT 1
#define MENU_MAIN_VOLUME 2
#define MENU_MAIN_AGCSPEED 3
#define MENU_MAIN_RF_POWER 4
#define MENU_MAIN_FM_SQL 5
#define MENU_MAIN_RF_GAIN 6

#define COLOR_BUTTON_ACTIVE COLOR_DGREEN
#define COLOR_BUTTON_INACTIVE rgb888torgb565(198, 202, 206)
#define COLOR_BUTTON_MENU rgb888torgb565(224, 145, 8)
#define COLOR_BUTTON_TEXT rgb888torgb565(0, 27, 51)

#define METER_WIDTH 172

typedef struct {
	bool Background;
	bool MainMenu;
	bool TopButtons;
	bool FreqInfo;
	bool StatusInfoGUI;
	bool StatusInfoBar;
	bool SystemMenu;
} DEF_LCD_UpdateQuery;

struct button_handler {
	uint16_t x1;
	uint16_t x2;
	uint16_t y1;
	uint16_t y2;
	void(*onClickHandler) (void);
};

extern IWDG_HandleTypeDef hiwdg;

extern void LCD_Init(void);
extern void LCD_doEvents(void);
extern void LCD_showError(char text[], bool redraw);
extern void LCD_redraw(void);
extern void LCD_checkTouchPad(void);
extern void LCD_Handler_SETTIME(void);

volatile extern DEF_LCD_UpdateQuery LCD_UpdateQuery;
volatile extern bool LCD_busy;
volatile extern bool LCD_bandMenuOpened;
volatile extern bool LCD_timeMenuOpened;
volatile extern bool LCD_modeMenuOpened;
volatile extern bool LCD_systemMenuOpened;
volatile extern bool LCD_mainMenuOpened;
volatile extern uint8_t TimeMenuSelection;
volatile extern uint8_t LCD_menu_main_index;
volatile extern bool LCD_NotchEdit;

#endif
