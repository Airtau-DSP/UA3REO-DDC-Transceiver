#include "settings.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "fpga.h"
#include "main.h"
#include "bands.h"

//W25Q16
uint8_t Write_Enable = W25Q16_COMMAND_Write_Enable;
uint8_t Erase_Chip = W25Q16_COMMAND_Erase_Chip;
uint8_t Sector_Erase = W25Q16_COMMAND_Sector_Erase;
uint8_t Page_Program = W25Q16_COMMAND_Page_Program;
uint8_t Read_Data = W25Q16_COMMAND_Read_Data;
uint8_t Address[3] = { 0x00 };

struct TRX_SETTINGS TRX;
bool NeedSaveSettings = false;

void LoadSettings(void)
{
	Flash_Read_Data();
	if (TRX.clean_flash != 170) //code to trace new clean flash
	{
		TRX.clean_flash = 170;
		TRX.VFO_A.Freq = 7100000;
		TRX.VFO_A.Mode = TRX_MODE_LSB;
		TRX.VFO_A.Filter_Width = 2700;
		TRX.VFO_B.Freq = 14150000;
		TRX.VFO_B.Mode = TRX_MODE_USB;
		TRX.VFO_B.Filter_Width = 2700;
		TRX.current_vfo = false; // A
		TRX.Preamp = false;
		TRX.AGC = true;
		TRX.ATT = false;
		TRX.LPF = true;
		TRX.BPF = true;
		TRX.TX_Amplifier = true;
		TRX.Notch = 0;
		TRX.DNR = false;
		TRX.Agc_speed = 3;
		TRX.LCD_menu_freq_index = MENU_FREQ_KHZ;
		TRX.BandMapEnabled = true;
		TRX.Volume = 20;
		TRX.InputType = 0; //0 - mic ; 1 - line ; 2 - usb
		TRX.Mute = false;
		TRX.Fast = false;
		TRX.CW_Filter = 500;
		TRX.SSB_Filter = 2700;
		TRX.FM_Filter = 15000;
		TRX.RF_Power = 20;
		TRX.FM_SQL_threshold = 1;
		TRX.RF_Gain = 50;
		for(uint8_t i=0;i<BANDS_COUNT;i++) TRX.saved_freq[i]=BANDS[i].startFreq+(BANDS[i].endFreq-BANDS[i].startFreq)/2;
		TRX.FFT_Zoom=1;
		TRX.AutoGain=false;
		//system settings
		TRX.FFT_Enabled = true;
		TRX.CW_GENERATOR_SHIFT_HZ=500;
		TRX.Touchpad_ax = 11.096;
		TRX.Touchpad_bx = -32;
		TRX.Touchpad_ay = -15.588235;
		TRX.Touchpad_by = 250;
		TRX.ENCODER_SLOW_RATE=35;
		TRX.LCD_Brightness=100;
		TRX.Standby_Time=180;
		TRX.Beeping=true;
		TRX.Key_timeout=1000;
		TRX.FFT_Averaging=4;
	}
}

VFO *CurrentVFO(void)
{
	if (!TRX.current_vfo)
		return &TRX.VFO_A;
	else
		return &TRX.VFO_B;
}

void SaveSettings(void)
{
	NeedSaveSettings = false;
	FPGA_NeedSendParams = true;
	Flash_Sector_Erase();
	//Flash_Erase_Chip();
	HAL_Delay(50);
	Flash_Write_Data();
}

void Flash_Erase_Chip(void)
{
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
	HAL_SPI_Transmit(&hspi1, &Write_Enable, 1, HAL_MAX_DELAY); // Write Enable Command
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high

	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
	HAL_SPI_Transmit(&hspi1, &Erase_Chip, 1, HAL_MAX_DELAY);   // Erase Chip Command
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
}

void Flash_Sector_Erase(void)
{
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
	HAL_SPI_Transmit(&hspi1, &Write_Enable, 1, HAL_MAX_DELAY); // Write Enable Command
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high

	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
	HAL_SPI_Transmit(&hspi1, &Sector_Erase, 1, HAL_MAX_DELAY);   // Erase Chip Command
	HAL_SPI_Transmit(&hspi1, Address, sizeof(Address), HAL_MAX_DELAY);      // Write Address ( The first address of flash module is 0x00000000 )
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
}

void Flash_Write_Data(void)
{
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
	HAL_SPI_Transmit(&hspi1, &Write_Enable, 1, HAL_MAX_DELAY); // Write Enable Command
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high

	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
	HAL_SPI_Transmit(&hspi1, &Page_Program, 1, HAL_MAX_DELAY); // Page Program Command
	HAL_SPI_Transmit(&hspi1, Address, sizeof(Address), HAL_MAX_DELAY);      // Write Address ( The first address of flash module is 0x00000000 )
	HAL_SPI_Transmit(&hspi1, (uint8_t*)&TRX, sizeof(TRX), HAL_MAX_DELAY);       // Write
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
}

void Flash_Read_Data(void)
{
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
	HAL_SPI_Transmit(&hspi1, &Read_Data, 1, HAL_MAX_DELAY);  // Read Command
	HAL_SPI_Transmit(&hspi1, Address, sizeof(Address), HAL_MAX_DELAY);    // Write Address
	HAL_SPI_Receive(&hspi1, (uint8_t*)&TRX, sizeof(TRX), HAL_MAX_DELAY);      // Read
	HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
}
