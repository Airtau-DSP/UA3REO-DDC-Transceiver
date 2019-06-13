#ifndef WIFI_H
#define WIFI_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define WIFI_AnswerBuffer 256
#define WIFI_CommandTimeout 1000

extern void WIFI_Init(void);

#endif
