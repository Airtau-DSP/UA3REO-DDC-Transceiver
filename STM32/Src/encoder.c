#include "stm32f4xx_hal.h"
#include "main.h"
#include "encoder.h"
#include "lcd.h"
#include "trx_manager.h"
#include "agc.h"
#include "settings.h"
#include "system_menu.h"
#include "functions.h"

int ENCODER_ALast = 0;
int ENCODER_AVal = 0;
int32_t ENCODER_slowler = 0;

void ENCODER_Init()
{
	ENCODER_ALast = HAL_GPIO_ReadPin(GPIOE, ENC_CLK_Pin);
}

void ENCODER_checkRotate(void) {
	ENCODER_AVal = HAL_GPIO_ReadPin(GPIOE, ENC_CLK_Pin);
	if (ENCODER_AVal != ENCODER_ALast) { // проверка на изменение значения на выводе А по сравнению с предыдущим запомненным, что означает, что вал повернулся
		ENCODER_ALast = ENCODER_AVal;
		// а чтобы определить направление вращения, нам понадобится вывод В.
		if (HAL_GPIO_ReadPin(GPIOE, ENC_DT_Pin) != ENCODER_AVal) {  // Если вывод A изменился первым - вращение по часовой стрелке
			ENCODER_slowler--;
			if (ENCODER_slowler < -TRX.ENCODER_SLOW_RATE)
			{
				ENCODER_Rotated(-1);
				ENCODER_slowler = 0;
			}
		}
		else {// иначе B изменил свое состояние первым - вращение против часовой стрелки
			ENCODER_slowler++;
			if (ENCODER_slowler > TRX.ENCODER_SLOW_RATE)
			{
				ENCODER_Rotated(1);
				ENCODER_slowler = 0;
			}
		}
	}
}

void ENCODER_Rotated(int direction) //энкодер повернули, здесь обработчик, direction -1 - влево, 1 - вправо
{
	if (LCD_systemMenuOpened && !LCD_timeMenuOpened)
	{
		eventRotateSystemMenu(direction);
		return;
	}
	if (!LCD_mainMenuOpened)
	{
		switch (TRX.LCD_menu_freq_index) {
		case MENU_FREQ_HZ:
			if (TRX.Fast)
				TRX_setFrequency(TRX_getFrequency() + 100 * direction);
			else
				TRX_setFrequency(TRX_getFrequency() + 10 * direction);
			break;
		case MENU_FREQ_KHZ:
			if (TRX.Fast)
				TRX_setFrequency(TRX_getFrequency() + 10000 * direction);
			else
				TRX_setFrequency(TRX_getFrequency() + 1000 * direction);
			break;
		case MENU_FREQ_MHZ:
			TRX_setFrequency(TRX_getFrequency() + 1000000 * direction);
			break;
		default:
			break;
		}
		LCD_displayFreqInfo();
	}
	if (LCD_mainMenuOpened)
	{
		if (LCD_timeMenuOpened)
		{
			uint32_t Time = RTC->TR;
			RTC_TimeTypeDef sTime;
			sTime.TimeFormat = RTC_HOURFORMAT12_PM;
			sTime.SubSeconds = 0;
			sTime.SecondFraction = 0;
			sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
			sTime.StoreOperation = RTC_STOREOPERATION_SET;
			sTime.Hours = (uint8_t)(((Time >> 20) & 0x03) * 10 + ((Time >> 16) & 0x0f));
			sTime.Minutes = (uint8_t)(((Time >> 12) & 0x07) * 10 + ((Time >> 8) & 0x0f));
			sTime.Seconds = (uint8_t)(((Time >> 4) & 0x07) * 10 + ((Time >> 0) & 0x0f));
			if (TimeMenuSelection == 0)
			{
				if (sTime.Hours == 0 && direction < 0) return;
				sTime.Hours = sTime.Hours + direction;
			}
			if (TimeMenuSelection == 1)
			{
				if (sTime.Minutes == 0 && direction < 0) return;
				sTime.Minutes = sTime.Minutes + direction;
			}
			if (TimeMenuSelection == 2)
			{
				if (sTime.Seconds == 0 && direction < 0) return;
				sTime.Seconds = sTime.Seconds + direction;
			}
			if (sTime.Hours >= 24) sTime.Hours = 0;
			if (sTime.Minutes >= 60) sTime.Minutes = 0;
			if (sTime.Seconds >= 60) sTime.Seconds = 0;
			HAL_RTC_DeInit(&hrtc);
			HAL_RTC_Init(&hrtc);
			HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			LCD_UpdateQuery.SystemMenu = true;
			return;
		}
		switch (LCD_menu_main_index) {
		case MENU_MAIN_VOLUME:
			TRX.Volume = TRX.Volume + direction;
			if (TRX.Volume < 1) TRX.Volume = 1;
			if (TRX.Volume > 100) TRX.Volume = 100;
			LCD_UpdateQuery.MainMenu = true;
			break;
		case MENU_MAIN_RF_GAIN:
			TRX.RF_Gain = TRX.RF_Gain + direction;
			if (TRX.RF_Gain < 1) TRX.RF_Gain = 1;
			if (TRX.RF_Gain > 250) TRX.RF_Gain = 250;
			LCD_UpdateQuery.MainMenu = true;
			break;
		case MENU_MAIN_FM_SQL:
			if (direction > 0 || TRX.FM_SQL_threshold > 0) TRX.FM_SQL_threshold = TRX.FM_SQL_threshold + direction;
			if (TRX.FM_SQL_threshold > 10) TRX.FM_SQL_threshold = 10;
			LCD_UpdateQuery.MainMenu = true;
			break;
		case MENU_MAIN_RF_POWER:
			TRX.RF_Power = TRX.RF_Power + direction;
			if (TRX.RF_Power < 1) TRX.RF_Power = 1;
			if (TRX.RF_Power > 100) TRX.RF_Power = 100;
			LCD_UpdateQuery.MainMenu = true;
			break;
		case MENU_MAIN_AGCSPEED:
			if (direction > 0 || TRX.Agc_speed > 0) TRX.Agc_speed = TRX.Agc_speed + direction;
			if (TRX.Agc_speed > 4) TRX.Agc_speed = 4;
			SetupAgcWdsp();
			LCD_UpdateQuery.MainMenu = true;
			break;
		default:
			break;
		}
		NeedSaveSettings = true;
	}
}
