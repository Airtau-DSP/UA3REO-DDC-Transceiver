#include "stm32f4xx_hal.h"
#include "xpt2046_spi.h"
#include "lcd_driver.h"
#include "main.h"
#include "../functions.h"
#include "../settings.h"

volatile bool TOUCH_InCalibrate = false;

static float32_t touch_x0, touch_x1, touch_x2, touch_x3, touch_y0, touch_y1, touch_y2, touch_y3;
static const int16_t xCenter[] = { CALIBRATE_OFFSET, LCD_WIDTH - CALIBRATE_OFFSET, CALIBRATE_OFFSET, LCD_WIDTH - CALIBRATE_OFFSET };
static const int16_t yCenter[] = { CALIBRATE_OFFSET, CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET };
static int16_t xPos[5], yPos[5];

extern IWDG_HandleTypeDef hiwdg;

void Init_XPT2046()
{
	Spi_Master_Transmit(0X80);
	Spi_Master_Transmit(0X00);
	Spi_Master_Transmit(0X00);
	delay_us(1000);
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

	//производим расчёт используя координаты полученные при калибровке
	//вычисляем коэффициенты влияния каждой из координат тачпада
	float32_t x0_coeff = 1 - (touch_x - touch_x0) / (touch_x1 - touch_x0);
	float32_t x1_coeff = 1 - x0_coeff;
	float32_t x2_coeff = 1 - (touch_x - touch_x2) / (touch_x3 - touch_x2);
	float32_t x3_coeff = 1 - x2_coeff;

	if (x0_coeff < 0) x0_coeff = 0;
	if (x1_coeff < 0) x1_coeff = 0;
	if (x2_coeff < 0) x2_coeff = 0;
	if (x3_coeff < 0) x3_coeff = 0;
	if (x0_coeff > 1) x0_coeff = 1;
	if (x1_coeff > 1) x1_coeff = 1;
	if (x2_coeff > 1) x2_coeff = 1;
	if (x3_coeff > 1) x3_coeff = 1;

	float32_t y0_coeff = (touch_y - touch_y2) / (touch_y0 - touch_y2);
	float32_t y1_coeff = (touch_y - touch_y3) / (touch_y1 - touch_y3);
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
	float32_t true_x = LCD_WIDTH * (q1 + q3);
	float32_t true_y = LCD_HEIGHT * (q2 + q3);

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
void Touch_Set_Coef(float32_t x0, float32_t y0, float32_t x1, float32_t y1, float32_t x2, float32_t y2, float32_t x3, float32_t y3)
{
	char dest[100];
	touch_x0 = x0;
	touch_y0 = y0;
	touch_x1 = x1;
	touch_y1 = y1;
	touch_x2 = x2;
	touch_y2 = y2;
	touch_x3 = x3;
	touch_y3 = y3;

	//sprintf(dest, "Set touchpad calibrate: x0 = %f  y0 = %f x1 = %f  y1 = %f x2 = %f  y2 = %f x3 = %f  y3 = %f\r\n", x0,y0,x1,y1,x2,y2,x3,y3);
	//sendToDebug_str(dest);
}

//функция калибровки
void Touch_Calibrate(void)
{
	uint16_t x, y;
	TOUCH_InCalibrate = true;
	//рисуем крестик в левом верхнем углу
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Callibration", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_drawLine(0, CALIBRATE_OFFSET, CALIBRATE_OFFSET * 2, CALIBRATE_OFFSET, 0x0000); //h
	LCDDriver_drawLine(CALIBRATE_OFFSET, 0, CALIBRATE_OFFSET, CALIBRATE_OFFSET * 2, 0x0000); //v

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		//ждать нажатия
		while (!isTouch()) { HAL_IWDG_Refresh(&hiwdg); }
		Get_Touch_XY(&x, &y, 100, 1);//производим измерения 100 раз
		if (x < 4090 && y < 4090)
		{
			xPos[0] = x;
			yPos[0] = y;
			break;
		}
	}

	LCDDriver_printText("Off   ", 50, 120, 0xFFE0, 0x0000, 2);
	//ждать отпускания
	while (isTouch());
	LCDDriver_printText("     ", 50, 120, 0xFFE0, 0x0000, 2);


	//рисуем крестик в правом верхнем углу
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Callibration", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_drawLine(LCD_WIDTH - CALIBRATE_OFFSET * 2, CALIBRATE_OFFSET, LCD_WIDTH, CALIBRATE_OFFSET, 0x0000); //h
	LCDDriver_drawLine(LCD_WIDTH - CALIBRATE_OFFSET, 0, LCD_WIDTH - CALIBRATE_OFFSET, CALIBRATE_OFFSET * 2, 0x0000); //v

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		//ждать нажатия
		while (!isTouch()) { HAL_IWDG_Refresh(&hiwdg); }
		Get_Touch_XY(&x, &y, 100, 1);//производим измерения 100 раз
		if (x < 4090 && y < 4090)
		{
			xPos[1] = x;
			yPos[1] = y;
			break;
		}
	}
	LCDDriver_printText("Off   ", 50, 120, 0xFFE0, 0x0000, 2);

	//ждать отпускания
	while (isTouch());
	LCDDriver_printText("     ", 50, 120, 0xFFE0, 0x0000, 2);


	//рисуем крестик в левом нижнем углу
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Callibration", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_drawLine(0, LCD_HEIGHT - CALIBRATE_OFFSET, CALIBRATE_OFFSET * 2, LCD_HEIGHT - CALIBRATE_OFFSET, 0x0000);	// h
	LCDDriver_drawLine(CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET * 2, CALIBRATE_OFFSET, LCD_HEIGHT, 0x0000);	// v

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		// ждать нажатия
		while (!isTouch()) { HAL_IWDG_Refresh(&hiwdg); }
		Get_Touch_XY(&x, &y, 100, 1);//производим измерения 100 раз
		if (x < 4090 && y < 4090)
		{
			xPos[2] = x;
			yPos[2] = y;
			break;
		}
	}
	LCDDriver_printText("Off   ", 50, 120, 0xFFE0, 0x0000, 2);

	// ждать отпускания
	while (isTouch());
	LCDDriver_printText("     ", 50, 120, 0xFFE0, 0x0000, 2);


	//рисуем крестик в правом нижнем углу
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Callibration", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_drawLine(LCD_WIDTH - CALIBRATE_OFFSET * 2, LCD_HEIGHT - CALIBRATE_OFFSET, LCD_WIDTH, LCD_HEIGHT - CALIBRATE_OFFSET, 0x0000);	// h
	LCDDriver_drawLine(LCD_WIDTH - CALIBRATE_OFFSET, LCD_HEIGHT - CALIBRATE_OFFSET * 2, LCD_WIDTH - CALIBRATE_OFFSET, LCD_HEIGHT, 0x0000);	// v

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		// ждать нажатия
		while (!isTouch()) { HAL_IWDG_Refresh(&hiwdg); }
		Get_Touch_XY(&x, &y, 100, 1);
		if (x < 4090 && y < 4090)
		{
			xPos[3] = x;
			yPos[3] = y;
			break;
		}
	}
	LCDDriver_printText("Off   ", 50, 120, 0xFFE0, 0x0000, 2);

	//ждать отпускания
	while (isTouch());
	LCDDriver_printText("     ", 50, 120, 0xFFE0, 0x0000, 2);

	//Расчёт коэффициентов
	//вычисляем границы значений тачпада по краям экрана (отступы калибровки по 35 пикселей)
	float32_t xc_top = xPos[0] + (xPos[1] - xPos[0]) / 2; //центр экрана по верхним калибровкам (160px)
	float32_t xc_bottom = xPos[2] + (xPos[3] - xPos[2]) / 2; //центр экрана по нижним калибровкам (160px)

	float32_t x0 = xPos[0] - ((xc_top - xPos[0]) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X верхнего-левого угла
	float32_t x1 = xPos[1] + ((xPos[1] - xc_top) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X верхнего-правого угла
	float32_t x2 = xPos[2] - ((xc_bottom - xPos[2]) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X нижнего-левого угла
	float32_t x3 = xPos[3] + ((xPos[3] - xc_bottom) / (LCD_WIDTH / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты X нижнего-правого угла

	float32_t yc_left = yPos[2] + (yPos[0] - yPos[2]) / 2; //центр экрана по левым калибровкам (120px)
	float32_t yc_right = yPos[3] + (yPos[1] - yPos[3]) / 2; //центр экрана по правым калибровкам (120px)

	float32_t y0 = yPos[0] + ((yPos[0] - yc_left) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y верхнего-левого угла
	float32_t y1 = yPos[1] + ((yPos[1] - yc_right) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y верхнего-правого угла
	float32_t y2 = yPos[2] - ((yc_left - yPos[2]) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y нижнего-левого угла
	float32_t y3 = yPos[3] - ((yc_right - yPos[3]) / (LCD_HEIGHT / 2 - CALIBRATE_OFFSET))*CALIBRATE_OFFSET; //координаты Y нижнего-правого угла

	// Применить коэффициенты
	Touch_Set_Coef(x0, y0, x1, y1, x2, y2, x3, y3);
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
		TRX.Calibrated = true;
		SaveSettings();
		LCDDriver_printText("Saved", 50, 100, 0xFFE0, 0x0000, 2);
	}
	else
	{
		Touch_Set_Coef(TRX.Touchpad_x0, TRX.Touchpad_y0, TRX.Touchpad_x1, TRX.Touchpad_y1, TRX.Touchpad_x2, TRX.Touchpad_y2, TRX.Touchpad_x3, TRX.Touchpad_y3);
		LCDDriver_printText("Cancelled", 50, 100, 0xFFE0, 0x0000, 2);
	}
	HAL_Delay(1000);	// 1 sec
	TOUCH_InCalibrate = false;
}
