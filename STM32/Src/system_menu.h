#ifndef SYSTEM_MENU_H
#define SYSTEM_MENU_H

#include "stm32f4xx.h"
#include <stdbool.h>

typedef enum 
{
	SYSMENU_BOOLEAN       = 0x01U,
	SYSMENU_RUN       = 0x02U,
	SYSMENU_UINT8       = 0x03U,
	SYSMENU_UINT16       = 0x04U,
	SYSMENU_UINT32       = 0x05U,
	SYSMENU_INT8       = 0x06U,
	SYSMENU_INT16       = 0x07U,
	SYSMENU_INT32       = 0x08U,
} SystemMenuType;

struct sysmenu_item_handler {
	char *title;
	SystemMenuType type;
	uint32_t *value;
	void (*menuHandler) (int8_t direction);
};

extern void drawSystemMenu(bool draw_background);
extern void eventClickSystemMenu(uint16_t x, uint16_t y);
extern void eventRotateSystemMenu(int8_t direction);
	
#endif
