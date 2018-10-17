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
#define I2C_DELAY for(int i=0;i<1000;i++) { __asm("nop"); };

uint8_t i2c_writeOneByte(uint8_t);
uint8_t i2c_readOneByte(uint8_t, uint8_t*);
void i2c_begin(void);
void i2c_beginTransmission_u8(uint8_t);
void i2c_beginTransmission_i(int);
uint8_t i2c_endTransmission(void);
uint8_t i2c_requestFrom_u8i(uint8_t, int);
uint8_t i2c_requestFrom_ii(int, int);
void i2c_write_u8(uint8_t);
void i2c_write_u8i(uint8_t*, int);
void i2c_write_i(int);
void i2c_write_ii(int*, int);
void i2c_write_c(char*);
uint8_t i2c_available(void);
uint8_t i2c_read(void);
void    i2c_start(void);
void    i2c_stop(void);
bool i2c_get_ack(void);
void    i2c_send_ack(void);
void    i2c_send_nack(void);
uint8_t   i2c_shift_in(void);
void    i2c_shift_out(uint8_t val);

#endif
