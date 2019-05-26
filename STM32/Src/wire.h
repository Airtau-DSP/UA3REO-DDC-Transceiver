#ifndef WIRE_h
#define WIRE_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define I2C_SDA_PORT WM8731_SCK_GPIO_Port
#define I2C_SDA_PIN WM8731_SDA_Pin
#define I2C_SCL_PORT WM8731_SDA_GPIO_Port
#define I2C_SCL_PIN WM8731_SCK_Pin

/* You must update the online docs if you change this value. */
#define WIRE_BUFSIZ 101

/* return codes from endTransmission() */
#define SUCCESS   0        /* transmission was successful */
#define EDATA     1        /* too much data */
#define ENACKADDR 2        /* received nack on transmit of address */
#define ENACKTRNS 3        /* received nack on transmit of data */
#define EOTHER    4        /* other error */

#define I2C_WRITE 0
#define I2C_READ  1
#define I2C_DELAY for(int wait_i=0;wait_i<100;wait_i++) { __asm("nop"); };

extern void i2c_begin(void);
extern void i2c_beginTransmission_u8(uint8_t);
extern void i2c_write_u8(uint8_t);
extern uint8_t i2c_endTransmission(void);

#endif
