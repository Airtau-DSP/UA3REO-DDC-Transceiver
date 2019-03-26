#ifndef SYSTEM_MENU_H
#define SYSTEM_MENU_H

#include "stm32f4xx.h"
#include <stdbool.h>

typedef enum 
{
  SYSMENU_INTEGER       = 0x00U,
	SYSMENU_BOOLEAN       = 0x01U,
	SYSMENU_RUN       = 0x02U,
} SystemMenuType;

extern void drawSystemMenu(bool draw_background);
extern void eventClickSystemMenu(uint16_t x, uint16_t y);
extern void eventRotateSystemMenu(int direction);
void drawSystemMenuElement(char* title, SystemMenuType type, uint32_t value);
	
#endif
