#ifndef WIFI_H
#define WIFI_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define WIFI_ANSWER_BUFFER_SIZE 512
#define WIFI_ANSWER_READBUFFER_SIZE 64
#define WIFI_COMMAND_TIMEOUT 10

typedef enum
{
	WIFI_UNDEFINED = 0x00U,
	WIFI_NOTFOUND = 0x01U,
	WIFI_INITED = 0x02U,
} WiFiState;

extern void WIFI_Init(void);

#endif
