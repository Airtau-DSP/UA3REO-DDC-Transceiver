#include "stm32f4xx_hal.h"
#include "xpt2046_spi.h"
#include "lcd_driver.h"
#include "main.h"
#include "../functions.h"
#include "../settings.h"

volatile bool TOUCH_InCalibrate = false;

static float32_t touch_x0, touch_x1, touch_x2, touch_x3, touch_x4, touch_x5, touch_x6, touch_x7, touch_x8, touch_y0, touch_y1, touch_y2, touch_y3, touch_y4, touch_y5, touch_y6, touch_y7, touch_y8;
static const int16_t xCenter[] = { CALIBRATE_OFFSET, LCD_WIDTH - CALIBRATE_OFFSET, CALIBRATE_OFFSET, LCD_WIDTH - CALIBRATE_OFFSET };
static const int16_t yCenter[] = { CALIBRATE_OFFSET, CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET };
static uint16_t xPos[9], yPos[9];

extern IWDG_HandleTypeDef hiwdg;

void Init_XPT2046()
{
	Spi_Master_Transmit(0X80);
	Spi_Master_Transmit(0X00);
	Spi_Master_Transmit(0X00);
	HAL_Delay(1);
}
///////////////////////////////////
uint8_t Spi_Master_Transmit(uint8_t out_data)
{
	uint8_t in_data = 0;
	HAL_SPI_TransmitReceive(&hspi2, &out_data, &in_data, 1, 0x1000);
	return in_data;

}
/////////////////////////////////

uint8_t isTouch(void)
{
	if (HAL_GPIO_ReadPin(LED_PEN_GPIO_Port, LED_PEN_Pin) == GPIO_PIN_RESET)
		return 1;	// прикосновение

	return 0;
}

uint16_t Get_Touch(uint8_t adress)
{
	uint16_t data = 0;
	//CS_TOUCH_LOW;//активируем XPT2046

		//отправляем запрос для получения интересющей нас координаты 
	Spi_Master_Transmit(adress);

	//считываем старший байт 
	data = Spi_Master_Transmit(0X00);
	data <<= 8;

	//считываем младший байт 
	data |= Spi_Master_Transmit(0X00);
	data >>= 3;
	//CS_TOUCH_HIGH;//деактивируем XPT2046

	return data;
}

///////////////////////////////
void Get_Touch_XY(volatile uint16_t *x_kor, volatile uint16_t *y_kor, uint8_t count_read, bool onCalibration)
{
	uint8_t i = 0;
	uint16_t tmpx, tmpy, touch_x, touch_y = 0;

	touch_x = Get_Touch(SPI_Y);//считываем координату Х
	delay_us(100);
	touch_y = Get_Touch(SPI_X);//считываем координату Y

	//считываем координаты опр. кол-во раз и каждый раз находим среднее значение
	//если одна из координат равна нулю второе значение тоже обнуляем и не учитываем его
	for (i = 0; i < count_read; i++)
	{
		tmpx = Get_Touch(SPI_Y);
		delay_us(100);
		tmpy = Get_Touch(SPI_X);

		if (tmpx == 0) tmpy = 0;
		else if (tmpy == 0) tmpx = 0;
		else
		{
			touch_x = (touch_x + tmpx) / 2;
			touch_y = (touch_y + tmpy) / 2;
		}
	}
	
	//определяем в какую из 4-х частей экрана попадает нажатие, запоминаем края нужной части
	uint8_t area = 0;
	float32_t area_touch_x0, area_touch_x1, area_touch_x2, area_touch_x3, area_touch_x4;
	float32_t area_touch_y0, area_touch_y1, area_touch_y2, area_touch_y3, area_touch_y4;
	if(beetween(touch_x0, touch_x1, touch_x) && beetween(touch_y0, touch_y3, touch_y)) //top-left
	{
		area = 1;
		area_touch_x0 = touch_x0;
		area_touch_x1 = touch_x1;
		area_touch_x2 = touch_x3;
		area_touch_x3 = touch_x8;
		area_touch_y0 = touch_y0;
		area_touch_y1 = touch_y1;
		area_touch_y2 = touch_y3;
		area_touch_y3 = touch_y8;
	}
	else if(beetween(touch_x1, touch_x2, touch_x) && beetween(touch_y2, touch_y4, touch_y)) //top-right
	{
		area = 2;
		area_touch_x0 = touch_x1;
		area_touch_x1 = touch_x2;
		area_touch_x2 = touch_x8;
		area_touch_x3 = touch_x4;
		area_touch_y0 = touch_y1;
		area_touch_y1 = touch_y2;
		area_touch_y2 = touch_y8;
		area_touch_y3 = touch_y4;
	}
	else if(beetween(touch_x5, touch_x6, touch_x) && beetween(touch_y3, touch_y5, touch_y)) //bottom-left
	{
		area = 3;
		area_touch_x0 = touch_x3;
		area_touch_x1 = touch_x8;
		area_touch_x2 = touch_x5;
		area_touch_x3 = touch_x6;
		area_touch_y0 = touch_y3;
		area_touch_y1 = touch_y8;
		area_touch_y2 = touch_y5;
		area_touch_y3 = touch_y6;
	}
	else if(beetween(touch_x6, touch_x7, touch_x) && beetween(touch_y4, touch_y7, touch_y)) //bottom-right
	{
		area = 4;
		area_touch_x0 = touch_x8;
		area_touch_x1 = touch_x4;
		area_touch_x2 = touch_x6;
		area_touch_x3 = touch_x7;
		area_touch_y0 = touch_y8;
		area_touch_y1 = touch_y4;
		area_touch_y2 = touch_y6;
		area_touch_y3 = touch_y7;
	}
	else //весь экран
	{
		area_touch_x0 = touch_x0;
		area_touch_x1 = touch_x2;
		area_touch_x2 = touch_x5;
		area_touch_x3 = touch_x7;
		area_touch_y0 = touch_y0;
		area_touch_y1 = touch_y2;
		area_touch_y2 = touch_y5;
		area_touch_y3 = touch_y8;
	}
	//
	
	//производим расчёт используя координаты полученные при калибровке
	//вычисляем коэффициенты влияния каждой из координат тачпада
	float32_t x0_coeff = 1 - (touch_x - area_touch_x0) / (area_touch_x1 - area_touch_x0);
	float32_t x1_coeff = 1 - x0_coeff;
	float32_t x2_coeff = 1 - (touch_x - area_touch_x2) / (area_touch_x3 - area_touch_x2);
	float32_t x3_coeff = 1 - x2_coeff;

	if (x0_coeff < 0) x0_coeff = 0;
	if (x1_coeff < 0) x1_coeff = 0;
	if (x2_coeff < 0) x2_coeff = 0;
	if (x3_coeff < 0) x3_coeff = 0;
	if (x0_coeff > 1) x0_coeff = 1;
	if (x1_coeff > 1) x1_coeff = 1;
	if (x2_coeff > 1) x2_coeff = 1;
	if (x3_coeff > 1) x3_coeff = 1;

	float32_t y0_coeff = (touch_y - area_touch_y2) / (area_touch_y0 - area_touch_y2);
	float32_t y1_coeff = (touch_y - area_touch_y3) / (area_touch_y1 - area_touch_y3);
	float32_t y2_coeff = 1 - y0_coeff;
	float32_t y3_coeff = 1 - y1_coeff;
	if (y0_coeff < 0) y0_coeff = 0;
	if (y1_coeff < 0) y1_coeff = 0;
	if (y2_coeff < 0) y2_coeff = 0;
	if (y3_coeff < 0) y3_coeff = 0;
	if (y0_coeff > 1) y0_coeff = 1;
	if (y1_coeff > 1) y1_coeff = 1;
	if (y2_coeff > 1) y2_coeff = 1;
	if (y3_coeff > 1) y3_coeff = 1;

	//вычисляем коэффициенты влияния каждого из угла тачпада
	float32_t q0 = x0_coeff * y0_coeff;
	float32_t q1 = x1_coeff * y1_coeff;
	float32_t q2 = x2_coeff * y2_coeff;
	float32_t q3 = x3_coeff * y3_coeff;

	//получаем значения координат на экране
	float32_t true_x = 0;
	float32_t true_y = 0;
	if(area==1)
	{
		true_x = (LCD_WIDTH / 2) * (q1 + q3);
		true_y = (LCD_HEIGHT / 2) * (q2 + q3);
	}
	else if(area==2)
	{
		true_x = (LCD_WIDTH / 2) + (LCD_WIDTH / 2) * (q1 + q3);
		true_y = (LCD_HEIGHT / 2) * (q2 + q3);
	}
	else if(area==3)
	{
		true_x = (LCD_WIDTH / 2) * (q1 + q3);
		true_y = (LCD_HEIGHT / 2) + (LCD_HEIGHT / 2) * (q2 + q3);
	}
	else if(area==4)
	{
		true_x = (LCD_WIDTH / 2) + (LCD_WIDTH / 2) * (q1 + q3);
		true_y = (LCD_HEIGHT / 2) + (LCD_HEIGHT / 2) * (q2 + q3);
	}
	else
	{
		true_x = LCD_WIDTH * (q1 + q3);
		true_y = LCD_HEIGHT * (q2 + q3);
	}

	if (onCalibration)
	{
		*x_kor = touch_x;
		*y_kor = touch_y;
	}
	else
	{
		*x_kor = true_x;
		*y_kor = true_y;
	}
}

//ф-ция устанавливает калибровочные коэффициенты
void Touch_Set_Coef(float32_t x0, float32_t y0, float32_t x1, float32_t y1, float32_t x2, float32_t y2, float32_t x3, float32_t y3, float32_t x4, float32_t y4, float32_t x5, float32_t y5, float32_t x6, float32_t y6, float32_t x7, float32_t y7, float32_t x8, float32_t y8)
{
	touch_x0 = x0;
	touch_y0 = y0;
	touch_x1 = x1;
	touch_y1 = y1;
	touch_x2 = x2;
	touch_y2 = y2;
	touch_x3 = x3;
	touch_y3 = y3;
	touch_x4 = x4;
	touch_y4 = y4;
	touch_x5 = x5;
	touch_y5 = y5;
	touch_x6 = x6;
	touch_y6 = y6;
	touch_x7 = x7;
	touch_y7 = y7;
	touch_x8 = x8;
	touch_y8 = y8;
	
	//char dest[300];
	//sprintf(dest, "Set touchpad calibrate:\r\n x0 = %f  y0 = %f\r\n x1 = %f  y1 = %f\r\n x2 = %f  y2 = %f\r\n x3 = %f  y3 = %f\r\n x4 = %f  y4 = %f\r\n x5 = %f  y5 = %f\r\n x6 = %f  y6 = %f\r\n x7 = %f  y7 = %f\r\n x8 = %f  y8 = %f\r\n", x0,y0,x1,y1,x2,y2,x3,y3,x4,y4,x5,y5,x6,y6,x7,y7,x8,y8);
	//sendToDebug_str(dest);
	//sendToDebug_flush();
}

static void Touch_DoCalibratePoint(uint16_t cal_x, uint16_t cal_y, uint16_t *ret_x, uint16_t *ret_y)
{
	uint16_t x, y;
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Callibration", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_drawLine(cal_x-CALIBRATE_OFFSET, cal_y, cal_x+CALIBRATE_OFFSET, cal_y, 0x0000); //h
	LCDDriver_drawLine(cal_x, cal_y-CALIBRATE_OFFSET, cal_x, cal_y+CALIBRATE_OFFSET, 0x0000); //v

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		//ждать нажатия
		while (!isTouch()) { HAL_IWDG_Refresh(&hiwdg); }
		Get_Touch_XY(&x, &y, 100, 1);//производим измерения 100 раз
		if (x < 4090 && y < 4090)
		{
			*ret_x = x;
			*ret_y = y;
			break;
		}
	}

	LCDDriver_printText("Off   ", 50, 120, 0xFFE0, 0x0000, 2);
	//ждать отпускания
	while (isTouch());
	LCDDriver_printText("     ", 50, 120, 0xFFE0, 0x0000, 2);
	
	//sendToDebug_float32(x,false);
	//sendToDebug_float32(y,false);
	//sendToDebug_newline();
	//sendToDebug_flush();
}

//функция калибровки
void Touch_Calibrate(void)
{
	uint16_t x, y;
	TOUCH_InCalibrate = true;
	
	Touch_DoCalibratePoint(CALIBRATE_OFFSET, CALIBRATE_OFFSET, &xPos[0], &yPos[0]); //рисуем крестик в левом верхнем углу
	Touch_DoCalibratePoint(LCD_WIDTH / 2, CALIBRATE_OFFSET, &xPos[1], &yPos[1]); //рисуем крестик вверху
	Touch_DoCalibratePoint(LCD_WIDTH - CALIBRATE_OFFSET, CALIBRATE_OFFSET, &xPos[2], &yPos[2]); //рисуем крестик в правом верхнем углу
	Touch_DoCalibratePoint(CALIBRATE_OFFSET, LCD_HEIGHT / 2, &xPos[3], &yPos[3]); //рисуем крестик слева
	Touch_DoCalibratePoint(LCD_WIDTH - CALIBRATE_OFFSET, LCD_HEIGHT / 2, &xPos[4], &yPos[4]); //рисуем крестик справа
	Touch_DoCalibratePoint(CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET, &xPos[5], &yPos[5]); //рисуем крестик в левом нижнем углу
	Touch_DoCalibratePoint(LCD_WIDTH / 2, LCD_HEIGHT - CALIBRATE_OFFSET, &xPos[6], &yPos[6]); //рисуем крестик снизу
	Touch_DoCalibratePoint(LCD_WIDTH - CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET, &xPos[7], &yPos[7]); //рисуем крестик в правом нижнем углу
	Touch_DoCalibratePoint(LCD_WIDTH / 2, LCD_HEIGHT / 2, &xPos[8], &yPos[8]); //рисуем крестик в центре

	//Расчёт коэффициентов
	//вычисляем границы значений тачпада по краям экрана (отступы калибровки по 35 пикселей)
	float32_t xc_top = xPos[1]; //центр экрана по верхним калибровкам (160px)
	float32_t xc_center = xPos[8]; //центр экрана X (160px)
	float32_t xc_bottom = xPos[6]; //центр экрана по нижним калибровкам (160px)

	float32_t x0 = xPos[0] - ((xc_top - xPos[0]) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X верхнего-левого угла
	float32_t x1 = xPos[1] - ((xc_center - xPos[1]) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты верхнего края по центру
	float32_t x2 = xPos[2] + ((xPos[2] - xc_top) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X верхнего-правого угла
	float32_t x3 = xPos[3] - ((xc_center - xPos[3]) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты левого края по центру
	float32_t x4 = xPos[4] + ((xPos[4] - xc_center) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты правого края по центру
	float32_t x5 = xPos[5] - ((xc_bottom - xPos[5]) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X нижнего-левого угла
	float32_t x6 = xPos[6] + ((xPos[6] - xc_center) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты нижнего края по центру
	float32_t x7 = xPos[7] + ((xPos[7] - xc_bottom) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X нижнего-правого угла
	float32_t x8 = xPos[8];  //координаты X центра

	float32_t yc_left = yPos[3]; //центр экрана по левым калибровкам (120px)
	float32_t yc_center = yPos[8]; //центр экрана Y (120px)
	float32_t yc_right = yPos[4]; //центр экрана по правым калибровкам (120px)

	float32_t y0 = yPos[0] + ((yPos[0] - yc_left) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y верхнего-левого угла
	float32_t y1 = yPos[1] - ((yc_center - yPos[1]) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты верхнего края по центру
	float32_t y2 = yPos[2] + ((yPos[2] - yc_right) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y верхнего-правого угла
	float32_t y3 = yPos[3] - ((yc_center - yPos[3]) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты левого края по центру
	float32_t y4 = yPos[4] + ((yPos[4] - yc_center) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты правого края по центру
	float32_t y5 = yPos[5] - ((yc_left - yPos[5]) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y нижнего-левого угла
	float32_t y6 = yPos[6] + ((yPos[6] - yc_center) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты нижнего края по центру
	float32_t y7 = yPos[7] - ((yc_right - yPos[7]) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y нижнего-правого угла
	float32_t y8 = yPos[8]; //координаты Y центра

	// Применить коэффициенты
	Touch_Set_Coef(x0, y0, x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6, x7, y7, x8, y8);
	// Сохранить в память
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Save settings?", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_Fill_RectWH(60, 135, 60, 40, COLOR_DGREEN);
	LCDDriver_printText("YES", 70, 150, COLOR_BLACK, COLOR_DGREEN, 2);
	LCDDriver_Fill_RectWH(180, 135, 60, 40, COLOR_RED);
	LCDDriver_printText("NO", 200, 150, COLOR_BLACK, COLOR_RED, 2);
	while (!isTouch()) { HAL_IWDG_Refresh(&hiwdg); }
	Get_Touch_XY(&x, &y, 1, 0);
	LCDDriver_Fill(COLOR_WHITE);
	if (x >= 60 && x <= 120 && y >= 135 && y <= 175)
	{
		TRX.Touchpad_x0 = x0;
		TRX.Touchpad_y0 = y0;
		TRX.Touchpad_x1 = x1;
		TRX.Touchpad_y1 = y1;
		TRX.Touchpad_x2 = x2;
		TRX.Touchpad_y2 = y2;
		TRX.Touchpad_x3 = x3;
		TRX.Touchpad_y3 = y3;
		TRX.Touchpad_x4 = x4;
		TRX.Touchpad_y4 = y4;
		TRX.Touchpad_x5 = x5;
		TRX.Touchpad_y5 = y5;
		TRX.Touchpad_x6 = x6;
		TRX.Touchpad_y6 = y6;
		TRX.Touchpad_x7 = x7;
		TRX.Touchpad_y7 = y7;
		TRX.Touchpad_x8 = x8;
		TRX.Touchpad_y8 = y8;
		TRX.Calibrated = true;
		SaveSettings();
		LCDDriver_printText("Saved", 50, 100, 0xFFE0, 0x0000, 2);
	}
	else
	{
		Touch_Set_Coef(TRX.Touchpad_x0, TRX.Touchpad_y0, TRX.Touchpad_x1, TRX.Touchpad_y1, TRX.Touchpad_x2, TRX.Touchpad_y2, TRX.Touchpad_x3, TRX.Touchpad_y3, TRX.Touchpad_x4, TRX.Touchpad_y4, TRX.Touchpad_x5, TRX.Touchpad_y5, TRX.Touchpad_x6, TRX.Touchpad_y6, TRX.Touchpad_x7, TRX.Touchpad_y7, TRX.Touchpad_x8, TRX.Touchpad_y8);
		LCDDriver_printText("Cancelled", 50, 100, 0xFFE0, 0x0000, 2);
	}
	HAL_Delay(1000);	// 1 sec
	TOUCH_InCalibrate = false;
}
