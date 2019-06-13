#include "wifi.h"
#include "stm32f4xx_hal.h"
#include "functions.h"
#include "settings.h"

extern UART_HandleTypeDef huart6;
extern IWDG_HandleTypeDef hiwdg;

static void WIFI_SendCommand(char* command);
static char* WIFI_Answer[WIFI_AnswerBuffer] = { 0 };
	
void WIFI_Init(void)
{
	//WIFI_SendCommand("AT+UART_CUR?\r\n");
	WIFI_SendCommand("AT+UART_CUR=115200,8,1,0,1\r\n");
	//WIFI_SendCommand("AT+UART_CUR?\r\n");
	WIFI_SendCommand("AT+GMR\r\n");
	sendToDebug_str((char*)WIFI_Answer);
	sendToDebug_newline();
}

static void WIFI_SendCommand(char* command)
{
	memset(WIFI_Answer,0x00,sizeof(WIFI_Answer));
	HAL_UART_Transmit(&huart6, (uint8_t*)command, strlen(command), WIFI_CommandTimeout);
	HAL_UART_Receive(&huart6, (uint8_t*)WIFI_Answer, WIFI_AnswerBuffer, WIFI_CommandTimeout);
	HAL_IWDG_Refresh(&hiwdg);
}
