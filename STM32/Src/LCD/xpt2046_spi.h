#ifndef _XPT2046_H_
#define _XPT2046_H_

#include "stm32f4xx_hal.h"

#define	SPI_Y 	0x90
#define	SPI_X 	0xD0

extern uint8_t Touch_Verify_Coef(void);
extern SPI_HandleTypeDef hspi2;

void delay_us(uint32_t us);
void Init_XPT2046(void);
uint8_t isTouch(void);
uint8_t Spi_Master_Transmit(uint8_t out_data);
void Touch_Calibrate(void);
uint16_t Get_Touch(uint8_t adress);
void Touch_Set_Coef(float _ax, int16_t _bx, float _ay, int16_t _by);
void Get_Touch_XY(volatile uint16_t *x_kor, volatile uint16_t *y_kor, uint8_t count_read, uint8_t isReadCorrected);

#endif

