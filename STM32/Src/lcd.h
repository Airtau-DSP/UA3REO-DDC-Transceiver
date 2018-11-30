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
#define MENU_MAIN_VOLUME 2
#define MENU_MAIN_MICGAIN 3
#define MENU_MAIN_AGCSPEED 4

#define TOUCHPAD_DELAY 100 //anti-bounce

struct button_handler {
	uint16_t x1;
	uint16_t x2;
	uint16_t y1;
	uint16_t y2;
	void(*handler) ();
};

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
void printButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* text, uint16_t back_color, uint16_t text_color, uint16_t active_color, bool active, void(*onclick) ());
void printMenuButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* text1, char* text2, bool active, bool switchable, void(*onclick) ());

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

//HANDLERS
void LCD_Handler_TUNE(void);
void LCD_Handler_MODE(void);
void LCD_Handler_BAND(void);
void LCD_Handler_MENU_PREAMP_UHF(void);
void LCD_Handler_MENU_PREAMP_HF(void);
void LCD_Handler_AGC(void);
void LCD_Handler_MUTE(void);
void LCD_Handler_MENU_BPF(void);
void LCD_Handler_MENU_ATT(void);
void LCD_Handler_MENU_MAP(void);
void LCD_Handler_MENU_LINEMIC(void);
void LCD_Handler_MENU(void);
void LCD_Handler_BAND_160(void);
void LCD_Handler_BAND_80(void);
void LCD_Handler_BAND_40(void);
void LCD_Handler_BAND_30(void);
void LCD_Handler_BAND_20(void);
void LCD_Handler_BAND_17(void);
void LCD_Handler_BAND_15(void);
void LCD_Handler_BAND_12(void);
void LCD_Handler_BAND_10(void);
void LCD_Handler_BAND_BACK(void);
void LCD_Handler_MODE_LSB(void);
void LCD_Handler_MODE_USB(void);
void LCD_Handler_MODE_IQ(void);
void LCD_Handler_MODE_CW(void);
void LCD_Handler_MODE_BACK(void);
void LCD_Handler_MODE_DIGL(void);
void LCD_Handler_MODE_DIGU(void);
void LCD_Handler_MODE_FM(void);
void LCD_Handler_MODE_AM(void);
void LCD_Handler_MODE_LOOP(void);
void LCD_Handler_MENU_BACK(void);
void LCD_Handler_MENU_VOLUME(void);
void LCD_Handler_MENU_MIC_G(void);
void LCD_Handler_MENU_AGC_S(void);
void LCD_Handler_LCD_Calibrate(void);
//

#endif
