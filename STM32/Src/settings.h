#ifndef SETTINGS_h
#define SETTINGS_h

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdbool.h>
#include "arm_math.h"

#define W25Q16_COMMAND_Write_Enable 0x06
#define W25Q16_COMMAND_Erase_Chip 0xC7
#define W25Q16_COMMAND_Sector_Erase 0x20
#define W25Q16_COMMAND_Page_Program 0x02
#define W25Q16_COMMAND_Read_Data 0x03

typedef struct {
	uint32_t Freq;
	uint8_t Mode;
	bool Agc;
	uint16_t Filter_Width;
} VFO;

extern struct TRX_SETTINGS {
	uint8_t clean_flash;
	bool current_vfo; // false - A; true - B
	VFO VFO_A;
	VFO VFO_B;
	bool Preamp;
	uint8_t Agc_speed;
	uint8_t LCD_menu_freq_index;
	bool BandMapEnabled;
	float Touchpad_ax;
	int16_t Touchpad_bx;
	float Touchpad_ay;
	int16_t Touchpad_by;
	uint8_t Volume;
	bool LineMicIn; //false - mic ; true - line
	bool Mute;
	bool Fast;
	uint16_t CW_Filter;
	uint16_t SSB_Filter;
	uint16_t FM_Filter;
	uint8_t RF_Power;
	bool FFT_Enabled;
	uint8_t	FM_SQL_threshold;
	uint8_t	RF_Gain;
} TRX;

extern bool NeedSaveSettings;
extern SPI_HandleTypeDef hspi1;
extern void LoadSettings(void);
extern void SaveSettings(void);
void Flash_Sector_Erase(void);
void Flash_Erase_Chip(void);
void Flash_Write_Data(void);
void Flash_Read_Data(void);
extern VFO *CurrentVFO(void);

#endif
