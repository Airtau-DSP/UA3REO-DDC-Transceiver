#include "wifi.h"
#include "stm32f4xx_hal.h"
#include "functions.h"
#include "settings.h"

extern UART_HandleTypeDef huart6;
extern IWDG_HandleTypeDef hiwdg;

static WiFiState WIFI_State = 0;
static char WIFI_Answer[WIFI_ANSWER_BUFFER_SIZE] = { 0 };
static uint16_t WIFI_Answer_ReadIndex = 0;
static char WIFI_readed[WIFI_ANSWER_READBUFFER_SIZE] = { 0 };

static void WIFI_SendCommand(char* command);
static bool WIFI_GetLine(void);

void WIFI_Init(void)
{
	HAL_UART_Receive_DMA(&huart6, (uint8_t*)WIFI_Answer, WIFI_ANSWER_BUFFER_SIZE);
	WIFI_SendCommand("AT+UART_CUR=115200,8,1,0,1\r\n"); //uart config
	WIFI_SendCommand("ATE0\r\n"); //echo off
	WIFI_SendCommand("AT+GMR\r\n"); //system info ESP8266
	/*
	AT version:1.6.2.0(Apr 13 2018 11:10:59)
	SDK version:2.2.1(6ab97e9)
	compile time:Jun  7 2018 19:34:27
	Bin version(Wroom 02):1.6.2
	*/	
	WIFI_SendCommand("AT\r\n");
	while(WIFI_GetLine())
	{
		if(strstr(WIFI_readed,"OK")==0)
		{
			WIFI_State = WIFI_INITED;
		}
	}
	if(WIFI_State==WIFI_INITED)
	{
		sendToDebug_str("WIFI Module Inited\r\n");
	}
	else
	{
		WIFI_State = WIFI_NOTFOUND;
		sendToDebug_str("WIFI Module Not Found\r\n");
	}
}

static void WIFI_SendCommand(char* command)
{
	WIFI_Answer_ReadIndex = WIFI_ANSWER_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart6.hdmarx);
	HAL_UART_Transmit_IT(&huart6, (uint8_t*)command, strlen(command));
	HAL_Delay(WIFI_COMMAND_TIMEOUT);
	HAL_IWDG_Refresh(&hiwdg);

#if 0	
	while(WIFI_GetLine())
	{
		sendToDebug_str(WIFI_readed);
		sendToDebug_newline();
	}
#endif
}

static bool WIFI_GetLine(void)
{
	uint16_t dma_index = WIFI_ANSWER_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart6.hdmarx);
	if(WIFI_Answer_ReadIndex == dma_index) return false;
	
	char tmp[WIFI_ANSWER_BUFFER_SIZE]= { 0 };
	char tmp2[WIFI_ANSWER_BUFFER_SIZE]= { 0 };
	
	if(WIFI_Answer_ReadIndex < dma_index)
	{
		strncpy (tmp, &WIFI_Answer[WIFI_Answer_ReadIndex], dma_index - WIFI_Answer_ReadIndex);
	}
	if(WIFI_Answer_ReadIndex > dma_index)
	{
		strncpy (tmp, &WIFI_Answer[WIFI_Answer_ReadIndex], WIFI_ANSWER_BUFFER_SIZE - WIFI_Answer_ReadIndex);
		strncpy (tmp2, WIFI_Answer, dma_index);
	}
	strcat(tmp,tmp2);
	
	char sep[2]="\n";
	char *istr;
	istr = strtok (tmp, sep);
	if(istr==NULL) 
		return false;
	strcpy(WIFI_readed, istr);
	
	WIFI_Answer_ReadIndex+=strlen(WIFI_readed)+strlen(sep);
	if(WIFI_Answer_ReadIndex > dma_index) WIFI_Answer_ReadIndex=dma_index;
	if(WIFI_Answer_ReadIndex >= WIFI_ANSWER_BUFFER_SIZE) WIFI_Answer_ReadIndex=WIFI_Answer_ReadIndex - WIFI_ANSWER_BUFFER_SIZE;
	return true;
}