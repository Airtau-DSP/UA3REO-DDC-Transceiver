#include "stm32f4xx_hal.h"
#include "xpt2046_spi.h"
#include "lcd_driver.h"
#include "main.h"
#include "../functions.h"
#include "../settings.h"

float ax, ay;
int16_t bx, by;
static float axc[2], ayc[2];
static int16_t bxc[2], byc[2];
static const int16_t xCenter[] = { 35, LCD_WIDTH - 35, 35, LCD_WIDTH - 35 };
static const int16_t yCenter[] = { 35, 35, LCD_HEIGHT - 35, LCD_HEIGHT - 35 };
static int16_t xPos[5], yPos[5];

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
void Get_Touch_XY(volatile uint16_t *x_kor, volatile uint16_t *y_kor, uint8_t count_read, uint8_t calibration_flag)
{
	uint8_t i = 0;
	uint16_t tmpx, tmpy, touch_x, touch_y = 0;

	touch_x = Get_Touch(SPI_Y);//считываем координату Х //Они перепутаны изза переворота))
	delay_us(100);
	touch_y = Get_Touch(SPI_X);//считываем координату Y //Они перепутаны изза переворота))

	//считываем координаты опр. кол-во раз и каждый раз находим среднее значение
	//если одна из координат равна нулю второе значение тоже обнуляем и не учитываем его
	for (i = 0; i < count_read; i++)
	{
		tmpx = Get_Touch(SPI_Y); //Они перепутаны изза переворота))
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

	//во время калибровки возращаем вычисленные выше значения, 
	//иначе производим расчёт используя коэф. полученные при калибровке
	if (!calibration_flag)
	{
		*x_kor = touch_x / ax + bx;
		*y_kor = touch_y / ay + by;
	}
	else
	{
		*x_kor = touch_x;
		*y_kor = touch_y;
	}
}

//ф-ция устанавливает калибровочные коэффициенты
void Touch_Set_Coef(float _ax, int16_t _bx, float _ay, int16_t _by)
{
	char dest[100];
	ax = _ax;
	bx = _bx;
	ay = _ay;
	by = _by;

	sprintf(dest, "Set touchpad calibrate: ax = %f  bx = %d  ay = %f  by = %d\r\n", ax, bx, ay, by);
	sendToDebug_str(dest);
}

//функция калибровки
void Touch_Calibrate(void)
{
	uint16_t x, y;

	//рисуем крестик в левом верхнем углу
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Callibration", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_drawLine(10, 10 + 25, 10 + 50, 10 + 25, 0x0000);
	LCDDriver_drawLine(10 + 25, 10, 10 + 25, 10 + 50, 0x0000);

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		//ждать нажатия
		while (!isTouch());
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
	LCDDriver_drawLine(LCD_WIDTH - 10 - 50, 10 + 25, LCD_WIDTH - 10 - 50 + 50, 10 + 25, 0x0000);
	LCDDriver_drawLine(LCD_WIDTH - 10 - 25, 10, LCD_WIDTH - 10 - 25, 10 + 50, 0x0000);

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		//ждать нажатия
		while (!isTouch());
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
	LCDDriver_drawLine(10, LCD_HEIGHT - 10 - 25, 10 + 50, LCD_HEIGHT - 10 - 25, 0x0000);	// hor
	LCDDriver_drawLine(10 + 25, LCD_HEIGHT - 10 - 50, 10 + 25, LCD_HEIGHT - 10 - 50 + 50, 0x0000);	// vert

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		// ждать нажатия
		while (!isTouch());
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
	LCDDriver_drawLine(LCD_WIDTH - 10 - 50, LCD_HEIGHT - 10 - 25, LCD_WIDTH - 10 - 50 + 50, LCD_HEIGHT - 10 - 25, 0x0000);	// hor
	LCDDriver_drawLine(LCD_WIDTH - 10 - 25, LCD_HEIGHT - 10 - 50, LCD_WIDTH - 10 - 25, LCD_HEIGHT - 10 - 50 + 50, 0x0000);	// vert

	LCDDriver_printText("Press", 50, 120, 0xFFE0, 0x0000, 2);
	while (1)
	{
		// ждать нажатия
		while (!isTouch());
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


	//Расчёт коэффициентов. 
	//xPos - значения полученные с помощью тачскринаx, xCenter - реальные координаты точек на дисплее
	axc[0] = (float)(xPos[3] - xPos[0]) / (xCenter[3] - xCenter[0]);
	bxc[0] = xCenter[0] - xPos[0] / axc[0];
	ayc[0] = (float)(yPos[3] - yPos[0]) / (yCenter[3] - yCenter[0]);
	byc[0] = yCenter[0] - yPos[0] / ayc[0];

	// Сохранить коэффициенты
	float tmp_ax = ax;
	int16_t tmp_bx = bx;
	float tmp_ay = ay;
	int16_t tmp_by = by;
	Touch_Set_Coef(axc[0], bxc[0], ayc[0], byc[0]);
	// Сохранить в память
	LCDDriver_Fill(COLOR_WHITE);
	LCDDriver_printText("Save settings?", 50, 100, 0xFFE0, 0x0000, 2);
	LCDDriver_Fill_RectWH(60, 135, 60, 40, COLOR_DGREEN);
	LCDDriver_printText("YES", 70, 150, COLOR_BLACK, COLOR_DGREEN, 2);
	LCDDriver_Fill_RectWH(180, 135, 60, 40, COLOR_RED);
	LCDDriver_printText("NO", 200, 150, COLOR_BLACK, COLOR_RED, 2);
	while (!isTouch());
	Get_Touch_XY(&x, &y, 1, 0);
	LCDDriver_Fill(COLOR_WHITE);
	if (x >= 60 && x <= 120 && y >= 135 && y <= 175)
	{
		TRX.Touchpad_ax = axc[0];
		TRX.Touchpad_bx = bxc[0];
		TRX.Touchpad_ay = ayc[0];
		TRX.Touchpad_by = byc[0];
		SaveSettings();
		LCDDriver_printText("Saved", 50, 100, 0xFFE0, 0x0000, 2);
	}
	else
	{
		Touch_Set_Coef(tmp_ax, tmp_bx, tmp_ay, tmp_by);
		LCDDriver_printText("Cancelled", 50, 100, 0xFFE0, 0x0000, 2);
	}
	HAL_Delay(1000);	// 1 sec
}
