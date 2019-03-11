#ifndef SYSTEM_MENU_H
#define SYSTEM_MENU_H

#include "stm32f4xx.h"

extern void drawSystemMenu(void);
extern void eventClickSystemMenu(uint16_t x, uint16_t y);
extern void eventRotateSystemMenu(int direction);
	
#endif
