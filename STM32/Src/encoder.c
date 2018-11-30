#include "stm32f4xx_hal.h"
#include "encoder.h"
#include "lcd.h"
#include "trx_manager.h"
#include "agc.h"
#include "settings.h"

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
			if (ENCODER_slowler < -ENCODER_RATE)
			{
				ENCODER_Rotated(-1);
				ENCODER_slowler = 0;
			}
		}
		else {// иначе B изменил свое состояние первым - вращение против часовой стрелки
			ENCODER_slowler++;
			if (ENCODER_slowler > ENCODER_RATE)
			{
				ENCODER_Rotated(1);
				ENCODER_slowler = 0;
			}
		}
	}
}

void ENCODER_Rotated(int direction) //энкодер повернули, здесь обработчик, direction -1 - влево, 1 - вправо
{
	if (!LCD_mainMenuOpened)
	{
		switch (TRX.LCD_menu_freq_index) {
		case MENU_FREQ_HZ:
			TRX_setFrequency(TRX_getFrequency() + 10 * direction);
			break;
		case MENU_FREQ_KHZ:
			TRX_setFrequency(TRX_getFrequency() + 1000 * direction);
			break;
		case MENU_FREQ_MHZ:
			TRX_setFrequency(TRX_getFrequency() + 1000000 * direction);
			break;
		default:
			break;
		}
		LCD_displayFreqInfo(false);
	}
	if (LCD_mainMenuOpened)
	{
		switch (LCD_menu_main_index) {
		case MENU_MAIN_VOLUME:
			TRX.Volume = TRX.Volume + direction;
			if (TRX.Volume < 1) TRX.Volume = 1;
			if (TRX.Volume > 200) TRX.Volume = 200;
			LCD_needRedrawMainMenu = true;
			break;
		case MENU_MAIN_MICGAIN:
			TRX.MicGain_level = TRX.MicGain_level + direction;
			if (TRX.MicGain_level < 1) TRX.MicGain_level = 1;
			if (TRX.MicGain_level > 99) TRX.MicGain_level = 99;
			LCD_needRedrawMainMenu = true;
			break;
		case MENU_MAIN_AGCSPEED:
			if (direction > 0 || TRX.Agc_speed > 0) TRX.Agc_speed = TRX.Agc_speed + direction;
			if (TRX.Agc_speed > 4) TRX.Agc_speed = 4;
			SetupAgcWdsp();
			LCD_needRedrawMainMenu = true;
			break;
		default:
			break;
		}
		NeedSaveSettings = true;
	}
}
