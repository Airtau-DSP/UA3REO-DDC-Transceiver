#ifndef _XPT2046_H_
#define _XPT2046_H_

#include "stm32f4xx_hal.h"
#include "arm_math.h"
#include <stdbool.h>

#define	SPI_Y 	0x90
#define	SPI_X 	0xD0

#define CALIBRATE_OFFSET 20

extern uint8_t Touch_Verify_Coef(void);
extern SPI_HandleTypeDef hspi2;

extern volatile bool TOUCH_InCalibrate;

void delay_us(uint32_t us);
void Init_XPT2046(void);
uint8_t isTouch(void);
uint8_t Spi_Master_Transmit(uint8_t out_data);
void Touch_Calibrate(void);
uint16_t Get_Touch(uint8_t adress);
void Touch_Set_Coef(float32_t x0, float32_t y0, float32_t x1, float32_t y1, float32_t x2, float32_t y2, float32_t x3, float32_t y3);
void Get_Touch_XY(volatile uint16_t *x_kor, volatile uint16_t *y_kor, uint8_t count_read, bool onCalibration);

#endif

