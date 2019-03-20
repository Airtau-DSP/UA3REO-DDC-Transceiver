#include <usbd_cdc.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "functions.h"
#include "settings.h"
#include "trx_manager.h"
#include <stdlib.h>
#include "lcd.h"
#include "fpga.h"

#define UART_BUFFER_SIZE 64

static void ua3reo_dev_cat_if_open         (void* itf, USBD_CDC_LineCodingType * lc);
static void ua3reo_dev_cat_if_close        (void* itf);
static void ua3reo_dev_cat_if_in_cmplt     (void* itf, uint8_t * pbuf, uint16_t length);
static void ua3reo_dev_cat_if_out_cmplt    (void* itf, uint8_t * pbuf, uint16_t length);
static void ua3reo_dev_cat_parseCommand(char* command);
static char rx_buffer[UART_BUFFER_SIZE]={0};
static uint8_t rx_buffer_head=0;
static uint8_t getFT450Mode(uint8_t VFO_Mode);
static uint8_t setFT450Mode(uint8_t FT450_Mode);

static const USBD_CDC_AppType ua3reo_dev_cat_app =
{
    .Name           = "CAT serial port as standard I/O",
    .Open           = ua3reo_dev_cat_if_open,
	  .Close          = ua3reo_dev_cat_if_close,
    .Received       = ua3reo_dev_cat_if_in_cmplt,
    .Transmitted    = ua3reo_dev_cat_if_out_cmplt,
};

USBD_CDC_IfHandleType _ua3reo_dev_cat_if = {
    .App = &ua3reo_dev_cat_app,
    .Base.AltCount = 1,
}, *const ua3reo_dev_cat_if = &_ua3reo_dev_cat_if;

static void ua3reo_dev_cat_if_open(void* itf, USBD_CDC_LineCodingType * lc)
{
	uint8_t buff[8];
	USBD_CDC_Receive(ua3reo_dev_cat_if, buff, 8);
}

static void ua3reo_dev_cat_if_close(void* itf)
{
}

static void ua3reo_dev_cat_if_in_cmplt(void* itf, uint8_t * pbuf, uint16_t length)
{
	uint8_t buff[length];
	if(length<=UART_BUFFER_SIZE)
	{
		for(uint16_t i=0;i<length;i++)
		{
			if(buff[i]!=0)
			{
				rx_buffer[rx_buffer_head]=buff[i];
				if(rx_buffer[rx_buffer_head]==';')
				{
					char commandLine[UART_BUFFER_SIZE]={0};
					memcpy(commandLine,rx_buffer,rx_buffer_head);
					ua3reo_dev_cat_parseCommand(commandLine);
					rx_buffer_head=0;
					memset(&rx_buffer,0,UART_BUFFER_SIZE);
					continue;
				}
				rx_buffer_head++;
				if(rx_buffer_head>=UART_BUFFER_SIZE)
				{
					rx_buffer_head=0;
					memset(&rx_buffer,0,UART_BUFFER_SIZE);
				}
			}
		}
	}
	USBD_CDC_Receive(ua3reo_dev_cat_if, buff, length);
}

static void ua3reo_dev_cat_if_out_cmplt(void* itf, uint8_t * pbuf, uint16_t length)
{
    //USBD_CDC_Receive(ua3reo_dev_cat_if,&pbuf, length);
}

static void CAT_Transmit(char* data)
{
	USBD_CDC_Transmit(ua3reo_dev_cat_if,(uint8_t*)data, strlen(data));
}

static void ua3reo_dev_cat_parseCommand(char* _command)
{
	//sendToDebug_str3("New CAT command: ",_command,"\r\n");
	if(strlen(_command)<2) return;
	char command[2]={0};
	strncpy(command, _command, 2);
	bool has_args=false;	
	char arguments[32]={0};
	char ctmp[32];
	if(strlen(_command)>2)
	{
		strncpy(arguments, _command+2, strlen(_command)-2);
		has_args=true;
	}
	
	if(strcmp(command,"AI")==0) // AUTO INFORMATION
	{
		if(!has_args)
		{
			CAT_Transmit("AI0;");
		}
		else
		{
			//ничего не делаем, автоуведомления и так не работают
		}
		return;
	}
	
	if(strcmp(command,"ID")==0) // IDENTIFICATION
	{
		if(!has_args)
		{
			CAT_Transmit("ID0241;");
		}
		else
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"FT")==0) // FUNCTION TX
	{
		if(!has_args)
		{
			CAT_Transmit("FT0;");
		}
		else
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}
	
	if(strcmp(command,"VS")==0) // VFO SELECT
	{
		if(!has_args)
		{
			if(!TRX.current_vfo)
				CAT_Transmit("VS0;");
			else
				CAT_Transmit("VS1;");
		}
		else
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}
	
	if(strcmp(command,"IF")==0) // INFORMATION
	{
		if(!has_args)
		{
			char answer[30]={0};
			strcat(answer, "IF001"); //memory channel
			if(TRX_getFrequency()<10000000) strcat(answer, "0");
			sprintf(ctmp, "%d", TRX_getFrequency());
			strcat(answer, ctmp); //freq
			strcat(answer, "+0000"); //clirifier offset
			strcat(answer, "0"); //RX clar off
			strcat(answer, "0"); //TX clar off
			sprintf(ctmp, "%d", getFT450Mode(TRX_getMode()));
			strcat(answer, ctmp); //mode
			strcat(answer, "0"); //VFO Memory
			strcat(answer, "0"); //CTCSS OFF
			strcat(answer, "00"); //TONE NUMBER
			strcat(answer, "0;"); //Simplex
			CAT_Transmit(answer);
		}
		else
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}
	
	if(strcmp(command,"FA")==0) // FREQUENCY VFO-A
	{
		if(!has_args)
		{
			char answer[30]={0};
			strcat(answer, "FA");
			if(TRX.VFO_A.Freq<10000000) strcat(answer, "0");
			sprintf(ctmp, "%d", TRX.VFO_A.Freq);
			strcat(answer, ctmp); //freq
			strcat(answer, ";");
			CAT_Transmit(answer);
		}
		else
		{
			if(TRX.current_vfo==0)
				TRX_setFrequency(atoi(arguments));
			TRX.VFO_A.Freq=atoi(arguments);
			LCD_displayFreqInfo();
		}
		return;
	}

	if(strcmp(command,"FB")==0) // FREQUENCY VFO-B
	{
		if(!has_args)
		{
			char answer[30]={0};
			strcat(answer, "FA");
			if(TRX.VFO_B.Freq<10000000) strcat(answer, "0");
			sprintf(ctmp, "%d", TRX.VFO_B.Freq);
			strcat(answer, ctmp); //freq
			strcat(answer, ";");
			CAT_Transmit(answer);
		}
		else
		{
			if(TRX.current_vfo==1)
				TRX_setFrequency(atoi(arguments));
			TRX.VFO_B.Freq=atoi(arguments);
			LCD_displayFreqInfo();
		}
		return;
	}
	
	if(strcmp(command,"RA")==0) // RF ATTENUATOR
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
				CAT_Transmit("RA00;");
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}
	
	if(strcmp(command,"PA")==0) // PRE-AMP
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				if(TRX.Preamp)
					CAT_Transmit("PA01;");
				else
					CAT_Transmit("PA00;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"GT")==0) // AGC FUNCTION
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				if(TRX.Agc_speed==0 || !CurrentVFO()->Agc) CAT_Transmit("GT00;");
				else if(TRX.Agc_speed==1) CAT_Transmit("GT04;");
				else if(TRX.Agc_speed==2) CAT_Transmit("GT03;");
				else if(TRX.Agc_speed==3) CAT_Transmit("GT02;");
				else if(TRX.Agc_speed==4) CAT_Transmit("GT01;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}
	
	if(strcmp(command,"MD")==0) // MODE
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				char answer[30]={0};
				strcat(answer, "MD0");
				sprintf(ctmp, "%d", getFT450Mode(TRX_getMode()));
				strcat(answer, ctmp); //mode
				strcat(answer, ";");
				CAT_Transmit(answer);
			}
			else
			{
				TRX_setMode(setFT450Mode(atoi(arguments)));
				LCD_redraw();
			}
		}
		return;
	}

	if(strcmp(command,"PC")==0) // POWER CONTROL
	{
		if(!has_args)
		{
			char answer[30]={0};
			strcat(answer, "PC");
			sprintf(ctmp, "%d", TRX.RF_Power);
			strcat(answer, ctmp);
			strcat(answer, ";");
			CAT_Transmit(answer);
		}
		else
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}
	
	if(strcmp(command,"SH")==0) // WIDTH
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				CAT_Transmit("SH016;");
			}
		}
		return;
	}

	if(strcmp(command,"NB")==0) // NOISE BLANKER
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				CAT_Transmit("NB00;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"NR")==0) // NOISE REDUCTION
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				CAT_Transmit("NR00;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"VX")==0) // VOX STATUS
	{
		if(!has_args)
		{
			CAT_Transmit("VX0;");
		}
		else
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"CT")==0) // CTCSS
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				CAT_Transmit("CT00;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"ML")==0) // MONITOR LEVEL
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				CAT_Transmit("ML00;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"BP")==0) // MANUAL NOTCH
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"00")==0)
			{
				CAT_Transmit("BP00000;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"BI")==0) // BREAK IN
	{
		if(!has_args)
		{
			CAT_Transmit("BI0;");
		}
		else
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"OS")==0) // OFFSET
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				CAT_Transmit("OS00;");
			}
			else
				sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		return;
	}

	if(strcmp(command,"NA")==0) // NARROW
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
				CAT_Transmit("NA00;");
		}
		return;
	}

	if(strcmp(command,"SM")==0) // READ S-METER
	{
		if(!has_args)
		{
			sendToDebug_str3("Unknown CAT arguments: ",_command,"\r\n");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
				CAT_Transmit("SM0100;");
		}
		return;
	}

	if(strcmp(command,"TX")==0) // TX SET
	{
		if(!has_args)
		{
			if(TRX_ptt_cat)
				CAT_Transmit("TX1;");
			else if(TRX_ptt_hard)
				CAT_Transmit("TX2;");
			else
				CAT_Transmit("TX0;");
		}
		else
		{
			if(strcmp(arguments,"0")==0)
			{
				TRX_ptt_cat=false;
			}			
			if(strcmp(arguments,"1")==0)
			{
				TRX_ptt_cat=true;
			}
		}
		return;
	}
	
	sendToDebug_str3("Unknown CAT command: ",_command,"\r\n");
	//sendToDebug_str2(command,"|\r\n");
	//sendToDebug_str2(arguments,"|\r\n");
}

static uint8_t getFT450Mode(uint8_t VFO_Mode)
{
	if(VFO_Mode==TRX_MODE_LSB) return 1;
	if(VFO_Mode==TRX_MODE_USB) return 2;
	if(VFO_Mode==TRX_MODE_IQ) return 8;
	if(VFO_Mode==TRX_MODE_CW_L) return 3;
	if(VFO_Mode==TRX_MODE_CW_U) return 3;
	if(VFO_Mode==TRX_MODE_DIGI_L) return 6;
	if(VFO_Mode==TRX_MODE_DIGI_U) return 9;
	if(VFO_Mode==TRX_MODE_NO_TX) return 8;
	if(VFO_Mode==TRX_MODE_NFM) return 4;
	if(VFO_Mode==TRX_MODE_WFM) return 4;
	if(VFO_Mode==TRX_MODE_AM) return 5;
	if(VFO_Mode==TRX_MODE_LOOPBACK) return 8;
	return 1;
}

static uint8_t setFT450Mode(uint8_t FT450_Mode)
{
	if(FT450_Mode==1) return TRX_MODE_LSB;
	if(FT450_Mode==2) return TRX_MODE_USB;
	if(FT450_Mode==8) return TRX_MODE_IQ;
	if(FT450_Mode==3) return TRX_MODE_CW_L;
	if(FT450_Mode==6) return TRX_MODE_DIGI_L;
	if(FT450_Mode==9) return TRX_MODE_DIGI_U;
	if(FT450_Mode==4) return TRX_MODE_NFM;
	if(FT450_Mode==5) return TRX_MODE_AM;
	return TRX_MODE_USB;
}
