#include "stm32f4xx_hal.h"
#include "functions.h"
#include "wire.h"

/* low level conventions:
 * - SDA/SCL idle high (expected high)
 * - always start with i2c_delay rather than end
 */

uint8_t i2c_rx_buf[WIRE_BUFSIZ] = { 0 };      /* receive buffer */
uint8_t i2c_rx_buf_idx = 0;               /* first unread idx in rx_buf */
uint8_t i2c_rx_buf_len = 0;               /* number of bytes read */
uint8_t i2c_tx_addr = 0;                  /* address transmitting to */
uint8_t i2c_tx_buf[WIRE_BUFSIZ] = { 0 };      /* transmit buffer */
uint8_t i2c_tx_buf_idx = 0;  /* next idx available in tx_buf, -1 overflow */
bool i2c_tx_buf_overflow = false;

void i2c_start(void) {
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
}

void i2c_stop(void) {
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

bool i2c_get_ack(void) {
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

void i2c_send_ack(void) {
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
}

void i2c_send_nack(void) {
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
}

uint8_t i2c_shift_in(void) {
	uint8_t data = 0;
	GPIO_InitTypeDef GPIO_InitStruct;

	int i;
	for (i = 0; i < 8; i++) {
		I2C_DELAY;
		HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
		I2C_DELAY;
		GPIO_InitStruct.Pin = I2C_SDA_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);
		data += HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN) << (7 - i);
		GPIO_InitStruct.Pin = I2C_SDA_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);
		I2C_DELAY;
		HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
	}

	return data;
}

void i2c_shift_out(uint8_t val) {
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

void i2c_beginTransmission_i(int slave_address) {
	i2c_beginTransmission_u8((uint8_t)slave_address);
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

uint8_t i2c_requestFrom_u8i(uint8_t address, int num_bytes) {
	if (num_bytes > WIRE_BUFSIZ) num_bytes = WIRE_BUFSIZ;

	i2c_rx_buf_idx = 0;
	i2c_rx_buf_len = 0;
	while (i2c_rx_buf_len < num_bytes) {
		if (!i2c_readOneByte(address, i2c_rx_buf + i2c_rx_buf_len)) i2c_rx_buf_len++;
		else break;
	}
	return i2c_rx_buf_len;
}

uint8_t i2c_requestFrom_ii(int address, int numBytes) {
	return i2c_requestFrom_u8i((uint8_t)address, (uint8_t)numBytes);
}

void i2c_write_u8(uint8_t value) {
	if (i2c_tx_buf_idx == WIRE_BUFSIZ) {
		i2c_tx_buf_overflow = true;
		return;
	}

	i2c_tx_buf[i2c_tx_buf_idx++] = value;
}

void i2c_write_u8l(uint8_t* buf, int len) {
	for (uint8_t i = 0; i < len; i++) i2c_write_u8(buf[i]);
}

void i2c_write_i(int value) {
	i2c_write_u8((uint8_t)value);
}

void i2c_write_bi(int* buf, int len) {
	i2c_write_u8l((uint8_t*)buf, (uint8_t)len);
}

void i2c_write_c(char* buf) {
	uint8_t *ptr = (uint8_t*)buf;
	while (*ptr) {
		i2c_write_u8(*ptr);
		ptr++;
	}
}

uint8_t i2c_available(void) {
	return i2c_rx_buf_len - i2c_rx_buf_idx;
}

uint8_t i2c_read(void) {
	if (i2c_rx_buf_idx == i2c_rx_buf_len) return 0;
	return i2c_rx_buf[i2c_rx_buf_idx++];
}

// private methods

uint8_t i2c_writeOneByte(uint8_t byte) {
	i2c_shift_out(byte);
	if (!i2c_get_ack()) return ENACKTRNS;
	return SUCCESS;
}

uint8_t i2c_readOneByte(uint8_t address, uint8_t *byte) {
	i2c_start();

	i2c_shift_out((address << 1) | I2C_READ);
	if (!i2c_get_ack()) return ENACKADDR;

	*byte = i2c_shift_in();

	i2c_send_nack();
	i2c_stop();

	return SUCCESS;      // no real way of knowing, but be optimistic!
}
