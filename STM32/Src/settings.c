#include "settings.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "fpga.h"
#include "main.h"

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
	if (TRX.clean_flash != 129) //code to trace new clean flash
	{
		TRX.clean_flash = 129;
		TRX.VFO_A.Freq = 7100000;
		TRX.VFO_A.Mode = TRX_MODE_LSB;
		TRX.VFO_A.Agc = true;
		TRX.VFO_A.Filter_Width=2700;
		TRX.VFO_B.Freq = 14150000;
		TRX.VFO_B.Mode = TRX_MODE_USB;
		TRX.VFO_B.Agc = true;
		TRX.VFO_B.Filter_Width=2700;
		TRX.current_vfo=false; // A
		TRX.Preamp = true;
		TRX.Agc_speed = 2;
		TRX.LCD_menu_freq_index = MENU_FREQ_KHZ;
		TRX.BandMapEnabled = true;
		TRX.Touchpad_ax = 11.096;
		TRX.Touchpad_bx = -32;
		TRX.Touchpad_ay = -15.588235;
		TRX.Touchpad_by = 250;
		TRX.Volume = 20;
		TRX.LineMicIn = false; //false - mic ; true - line
		TRX.Mute = false;
		TRX.Fast = false;
		TRX.CW_Filter=500;
		TRX.SSB_Filter=2700;
		TRX.FM_Filter=15000;
		TRX.RF_Power=25;
		TRX.FFT_Enabled=true;
		TRX.FM_SQL_threshold=1;
		TRX.RF_Gain=50;
	}
}

VFO *CurrentVFO(void)
{
	if(!TRX.current_vfo)
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
