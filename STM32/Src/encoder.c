#include "stm32f4xx_hal.h"
#include "encoder.h"
#include "lcd.h"
#include "trx_manager.h"
#include "agc.h"

int ENCODER_ALast=0;
int ENCODER_AVal=0;
uint32_t ENCODER_last_micros_clickdebouncer=0;

void ENCODER_Init()
{
  ENCODER_ALast = HAL_GPIO_ReadPin(GPIOE,ENC_CLK_Pin);
}

void ENCODER_checkRotate(void) {
  ENCODER_AVal = HAL_GPIO_ReadPin(GPIOE,ENC_CLK_Pin);
  if (ENCODER_AVal != ENCODER_ALast) { // проверка на изменение значения на выводе А по сравнению с предыдущим запомненным, что означает, что вал повернулся
    ENCODER_ALast = ENCODER_AVal;
    // а чтобы определить направление вращения, нам понадобится вывод В.
    if (HAL_GPIO_ReadPin(GPIOE,ENC_DT_Pin) != ENCODER_AVal) {  // Если вывод A изменился первым - вращение по часовой стрелке
      ENCODER_Rotated(1);
    } else {// иначе B изменил свое состояние первым - вращение против часовой стрелки
      ENCODER_Rotated(-1);
    }
  }
}

void ENCODER_checkClick(void) { //смена разрядности переключения частоты валкодером
  if (HAL_GetTick() - ENCODER_last_micros_clickdebouncer >= 500) //защита от дребезга контактов
	{
    if (HAL_GPIO_ReadPin(GPIOE,ENC_SW_Pin) == GPIO_PIN_RESET)
    {
      if (LCD_mainMenuOpened)
      {
        LCD_menu_main_index = LCD_menu_main_index + 1;
        if (LCD_menu_main_index > MENU_MAIN_COUNT) LCD_menu_main_index = 1;
        if (LCD_menu_main_index < 1) LCD_menu_main_index = 1;
        LCD_displayMainMenu();
      }
      if (!LCD_mainMenuOpened)
      {
        LCD_menu_freq_index = LCD_menu_freq_index + 1;
        if (LCD_menu_freq_index > MENU_FREQ_COUNT) LCD_menu_freq_index = 1;
        if (LCD_menu_freq_index < 1) LCD_menu_freq_index = 1;
        LCD_last_showed_freq = 0;
      }
    }
    ENCODER_last_micros_clickdebouncer = HAL_GetTick();
  }
}

void ENCODER_Rotated(int direction) //энкодер повернули, здесь обработчик, direction -1 - влево, 1 - вправо
{
  if (!LCD_mainMenuOpened)
  {
    switch (LCD_menu_freq_index) {
      case MENU_FREQ_HZ:
        TRX_setFrequency(TRX_getFrequency() + 50 * direction);
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
  }
  if (LCD_mainMenuOpened)
  {
    switch (LCD_menu_main_index) {
			case MENU_MAIN_EXIT:
        LCD_needRedrawMainMenu=true;
				LCD_mainMenuOpened = false;
				LCD_redraw();
        break;
      case MENU_MAIN_GAIN:
        TRX_gain_level = TRX_gain_level + direction;
				if (TRX_agc_speed < 1) TRX_agc_speed = 1;
				if (TRX_gain_level > 20) TRX_gain_level = 20;
        LCD_needRedrawMainMenu=true;
        break;
      case MENU_MAIN_AGCSPEED:
        if (direction > 0 || TRX_agc_speed > 0) TRX_agc_speed = TRX_agc_speed + direction;
				if (TRX_agc_speed > 4) TRX_agc_speed = 4;
				SetupAgcWdsp();
        LCD_needRedrawMainMenu=true;
        break;
      default:
        break;
    }
  }
}
