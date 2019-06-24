#ifndef WIFI_H
#define WIFI_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define WIFI_ANSWER_BUFFER_SIZE 512
#define WIFI_COMMAND_DELAY 100
#define WIFI_COMMAND_TIMEOUT 5000

typedef enum
{
	WIFI_UNDEFINED = 0x00U,
	WIFI_NOTFOUND = 0x01U,
	WIFI_INITED = 0x02U,
	WIFI_CONNECTING = 0x03U,
	WIFI_READY = 0x04U,
	WIFI_PROCESS_COMMAND = 0x05U,
	WIFI_TIMEOUT = 0x06U,
} WiFiState;

extern volatile WiFiState WIFI_State;
	
extern void WIFI_Init(void);
extern void WIFI_ProcessAnswer(void);
extern uint32_t WIFI_GetSNMPTime(void);
extern void WIFI_ListAP(void);
extern void WIFI_GetIP(void);
extern void WIFI_GetStatus(void);

#endif
