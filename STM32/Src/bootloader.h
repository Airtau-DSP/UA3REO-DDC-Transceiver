#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "stm32f4xx.h"
#include "stm32f4xx_hal_conf.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_gpio.h"

extern void JumpToBootloader(void);
extern void checkBootloaderButton(void);

#endif
