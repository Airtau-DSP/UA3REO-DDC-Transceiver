#include "stm32f4xx_hal.h"
#include "main.h"
#include "functions.h"
#include "wire.h"

/* low level conventions:
 * - SDA/SCL idle high (expected high)
 * - always start with i2c_delay rather than end
 */

static uint8_t i2c_rx_buf_idx = 0;               /* first unread idx in rx_buf */
static uint8_t i2c_rx_buf_len = 0;               /* number of bytes read */
static uint8_t i2c_tx_addr = 0;                  /* address transmitting to */
static uint8_t i2c_tx_buf[WIRE_BUFSIZ] = { 0 };      /* transmit buffer */
static uint8_t i2c_tx_buf_idx = 0;  /* next idx available in tx_buf, -1 overflow */
static bool i2c_tx_buf_overflow = false;

static uint8_t i2c_writeOneByte(uint8_t);
static void    i2c_start(void);
static void    i2c_stop(void);
static bool i2c_get_ack(void);
static void    i2c_shift_out(uint8_t val);

static void i2c_start(void) {
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
}

static void i2c_stop(void) {
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = I2C_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	I2C_DELAY;
	I2C_DELAY;
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
	I2C_DELAY;
}

static bool i2c_get_ack(void) {
	GPIO_InitTypeDef GPIO_InitStruct;
	int time = 0;
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
	GPIO_InitStruct.Pin = I2C_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);

	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	I2C_DELAY;

	while (HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN))
	{
		time++;
		if (time > 50)
		{
			i2c_stop();
			return false;
		}
		I2C_DELAY;
	}

	GPIO_InitStruct.Pin = I2C_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);

	//I2C_DELAY;
	//HAL_Delay(1);
	return true;
}

static void i2c_shift_out(uint8_t val) {
	int i;
	GPIO_InitTypeDef GPIO_InitStruct;
	for (i = 0; i < 8; i++) {

		I2C_DELAY;
		HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, (GPIO_PinState)!!(val & (1 << (7 - i))));
		GPIO_InitStruct.Pin = I2C_SDA_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);

		I2C_DELAY;
		HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);

		I2C_DELAY;
		I2C_DELAY;
		HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
	}
}

/*
 * Joins I2C bus as master on given SDA and SCL pins.
 */
void i2c_begin() {
	GPIO_InitTypeDef GPIO_InitStruct;

	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);
}

void i2c_beginTransmission_u8(uint8_t slave_address) {
	i2c_tx_addr = slave_address;
	i2c_tx_buf_idx = 0;
	i2c_tx_buf_overflow = false;
	i2c_rx_buf_idx = 0;
	i2c_rx_buf_len = 0;
}

uint8_t i2c_endTransmission(void) {
	if (i2c_tx_buf_overflow) return EDATA;
	i2c_start();

	//I2C_DELAY;
	i2c_shift_out((i2c_tx_addr << 1) | I2C_WRITE);
	if (!i2c_get_ack()) return ENACKADDR;

	// shift out the address we're transmitting to
	for (uint8_t i = 0; i < i2c_tx_buf_idx; i++) {
		uint8_t ret = i2c_writeOneByte(i2c_tx_buf[i]);
		if (ret) return ret;    // SUCCESS is 0
	}
	I2C_DELAY;
	I2C_DELAY;
	i2c_stop();

	i2c_tx_buf_idx = 0;
	i2c_tx_buf_overflow = false;
	return SUCCESS;
}

void i2c_write_u8(uint8_t value) {
	if (i2c_tx_buf_idx == WIRE_BUFSIZ) {
		i2c_tx_buf_overflow = true;
		return;
	}

	i2c_tx_buf[i2c_tx_buf_idx++] = value;
}

// private methods
static uint8_t i2c_writeOneByte(uint8_t byte) {
	i2c_shift_out(byte);
	if (!i2c_get_ack()) return ENACKTRNS;
	return SUCCESS;
}

