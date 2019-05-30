#include "bootloader.h"
#include "usb_device.h"
#include "main.h"
#include "lcd.h"

void checkBootloaderButton(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOE_CLK_ENABLE();
	GPIO_InitStruct.Pin = K1_Pin | K0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	//K0 PE4 K1 PE3
	if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET || HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET) JumpToBootloader(); //Go to bootloader if (K0 PE4) pressed
}

/**
 * Function to perform jump to system memory boot from user application
 *
 * Call function when you want to jump to system memory
 */
void JumpToBootloader(void) {
	void(*SysMemBootJump)(void);

	/**
	 * Step: Set system memory address.
	 *       For STM32F429, system memory is on 0x1FFF 0000
	 *       For other families, check AN2606 document table 110 with descriptions of memory addresses
	 */
	volatile uint32_t addr = 0x1FFF0000;
	if (TRX_inited)
		LCD_showError("Flash DFU mode", false);
	//turn on LCD backlight
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = LCD_BACKLIGT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(LCD_BACKLIGT_GPIO_Port, &GPIO_InitStruct);
	HAL_GPIO_WritePin(LCD_BACKLIGT_GPIO_Port, LCD_BACKLIGT_Pin, 0);
	//prepare cpu
	MX_USB_DevDisconnect();
	hiwdg.Init.Reload = 0;
	for (uint8_t i = 0; i < 255; i++)
		HAL_NVIC_DisableIRQ(i);
	HAL_RCC_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
	//go to bootloader
	SYSCFG->MEMRMP = 0x01;
	SysMemBootJump = (void(*)(void)) (*((uint32_t *)(addr + 4)));
	__set_MSP(*(uint32_t *)addr);
	SysMemBootJump();
}
