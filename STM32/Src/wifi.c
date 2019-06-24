#include "wifi.h"
#include "stm32f4xx_hal.h"
#include "functions.h"
#include "settings.h"

extern UART_HandleTypeDef huart6;
extern IWDG_HandleTypeDef hiwdg;

volatile WiFiState WIFI_State = 0;
static char WIFI_Answer[WIFI_ANSWER_BUFFER_SIZE] = { 0 };
static char WIFI_readed[WIFI_ANSWER_BUFFER_SIZE] = { 0 };
static char tmp[WIFI_ANSWER_BUFFER_SIZE] = { 0 };
static char tmp2[WIFI_ANSWER_BUFFER_SIZE] = { 0 };
static uint16_t WIFI_Answer_ReadIndex = 0;
static uint32_t commandStartTime = 0;
	
static void WIFI_SendCommand(char* command);
static bool WIFI_GetLine(void);

void WIFI_Init(void)
{
	HAL_UART_Receive_DMA(&huart6, (uint8_t*)WIFI_Answer, WIFI_ANSWER_BUFFER_SIZE);
	WIFI_SendCommand("AT+UART_CUR=115200,8,1,0,1\r\n"); //uart config
	WIFI_SendCommand("ATE0\r\n"); //echo off
	WIFI_SendCommand("AT+GMR\r\n"); //system info ESP8266
	while(WIFI_GetLine()) {}
	/*
	AT version:1.6.2.0(Apr 13 2018 11:10:59)
	SDK version:2.2.1(6ab97e9)
	compile time:Jun  7 2018 19:34:27
	Bin version(Wroom 02):1.6.2
	*/	
	WIFI_SendCommand("AT\r\n");
	while(WIFI_GetLine())
	{
		if(strstr(WIFI_readed,"OK")!=NULL)
		{
			sendToDebug_str("WIFI Module Inited\r\n");
			WIFI_State = WIFI_INITED;
		}
	}
	if(WIFI_State==WIFI_UNDEFINED)
	{
		WIFI_State = WIFI_NOTFOUND;
		sendToDebug_str("WIFI Module Not Found\r\n");
	}
}

void WIFI_ProcessAnswer(void)
{
	if(WIFI_State == WIFI_NOTFOUND || WIFI_State == WIFI_UNDEFINED) return;
	HAL_IWDG_Refresh(&hiwdg);
	
	switch(WIFI_State)
	{
		case WIFI_INITED:
			//sendToDebug_str("WIFI_DEBUG: START CONNECTING\r\n");
			WIFI_State = WIFI_CONNECTING;
			//WIFI_SendCommand("AT+RFPOWER=82\r\n"); //rf power
			WIFI_SendCommand("AT+CWMODE_CUR=1\r\n"); //station mode
			WIFI_SendCommand("AT+CWDHCP_CUR=1,1\r\n"); //DHCP
			WIFI_SendCommand("AT+CWAUTOCONN=1\r\n"); //AUTOCONNECT
			WIFI_SendCommand("AT+CWHOSTNAME=\"UA3REO\"\r\n"); //Hostname
			WIFI_SendCommand("AT+CWCOUNTRY_CUR=1,\"RU\",1,13\r\n"); //Country
			WIFI_SendCommand("AT+CIPSNTPCFG=1,3,\"us.pool.ntp.org\"\r\n"); //configure SNMP
			WIFI_SendCommand("AT+CWJAP_CUR=\"Xeon\",\"password\"\r\n"); //connect to AP
			while(WIFI_GetLine()) {}
			break;
				
		case WIFI_CONNECTING:
			WIFI_GetLine();
			if(strstr(WIFI_readed, "GOT IP")!=NULL)
				WIFI_State = WIFI_READY;
			if(strstr(WIFI_readed, "WIFI DISCONNECT")!=NULL)
				WIFI_State = WIFI_INITED;
			if(strstr(WIFI_readed, "FAIL")!=NULL)
				WIFI_State = WIFI_INITED;
			break;
			
		case WIFI_READY:
			//sendToDebug_str("WIFI_DEBUG: READY\r\n");
			break;
		
		case WIFI_PROCESS_COMMAND:
			WIFI_GetLine();
			if((HAL_GetTick() - commandStartTime) > WIFI_COMMAND_TIMEOUT)
				WIFI_State = WIFI_TIMEOUT;
			if(strstr(WIFI_readed, "OK")!=NULL)
				WIFI_State = WIFI_READY;
			break;
			
		case WIFI_TIMEOUT:
			WIFI_GetLine();
			WIFI_State = WIFI_READY;
			break;
		
		default:
			break;
	}
}

uint32_t WIFI_GetSNMPTime(void)
{
	if(WIFI_State != WIFI_READY) return 0;
	uint32_t ret=0;
	WIFI_State = WIFI_PROCESS_COMMAND;
	WIFI_SendCommand("AT+CIPSNTPTIME?\r\n"); //get SNMP time
	return ret;
}

void WIFI_ListAP(void)
{
	if(WIFI_State != WIFI_READY) return;
	WIFI_State = WIFI_PROCESS_COMMAND;
	WIFI_SendCommand("AT+CWLAP\r\n"); //List AP
}

void WIFI_GetIP(void)
{
	if(WIFI_State != WIFI_READY) return;
	WIFI_State = WIFI_PROCESS_COMMAND;
	WIFI_SendCommand("AT+CIPSTA_CUR?\r\n"); //get ip
}

void WIFI_GetStatus(void)
{
	if(WIFI_State != WIFI_READY) return;
	WIFI_State = WIFI_PROCESS_COMMAND;
	WIFI_SendCommand("AT+CIPSTATUS\r\n"); //connection status
}

static void WIFI_SendCommand(char* command)
{
	//WIFI_Answer_ReadIndex = WIFI_ANSWER_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart6.hdmarx);
	HAL_UART_Transmit_IT(&huart6, (uint8_t*)command, strlen(command));
	commandStartTime = HAL_GetTick();
	HAL_Delay(WIFI_COMMAND_DELAY);
	HAL_IWDG_Refresh(&hiwdg);
}

static bool WIFI_GetLine(void)
{
	memset(WIFI_readed, 0x00, sizeof(WIFI_readed));
	memset(tmp, 0x00, sizeof(tmp));
	memset(tmp2, 0x00, sizeof(tmp2));
	uint16_t dma_index = WIFI_ANSWER_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart6.hdmarx);
	if(WIFI_Answer_ReadIndex == dma_index) return false;
	
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
	if(strlen(tmp)==0) 
		return false;
	
	char *sep="\n";
	char *istr;
	istr = strstr (tmp, sep);
	if(istr == NULL) 
		return false;
	uint16_t len = istr - tmp + strlen(sep);
	strncpy(WIFI_readed, tmp, len);
	
	WIFI_Answer_ReadIndex += len;
	if(WIFI_Answer_ReadIndex > dma_index) WIFI_Answer_ReadIndex=dma_index;
	if(WIFI_Answer_ReadIndex >= WIFI_ANSWER_BUFFER_SIZE) WIFI_Answer_ReadIndex=WIFI_Answer_ReadIndex - WIFI_ANSWER_BUFFER_SIZE;

#if 0	//DEBUG
	sendToDebug_str2("WIFI_DEBUG: ", WIFI_readed);
	sendToDebug_flush();
#endif
	
	return true;
}