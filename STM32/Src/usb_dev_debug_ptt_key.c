#include <usbd_cdc.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CDC_TX_FIFO_BUFFER_SIZE 1024
uint8_t cdc_tx_fifo[CDC_TX_FIFO_BUFFER_SIZE]={0};
uint16_t cdc_tx_fifo_head=0;

static void ua3reo_dev_debug_ptt_key_if_open         (void* itf, USBD_CDC_LineCodingType * lc);
static void ua3reo_dev_debug_ptt_key_if_close        (void* itf);
static void ua3reo_dev_debug_ptt_key_if_in_cmplt     (void* itf, uint8_t * pbuf, uint16_t length);
static void ua3reo_dev_debug_ptt_key_if_out_cmplt    (void* itf, uint8_t * pbuf, uint16_t length);

static const USBD_CDC_AppType ua3reo_dev_debug_ptt_key_app =
{
    .Name           = "Debug/PTT/KEY serial port as standard I/O",
    .Open           = ua3reo_dev_debug_ptt_key_if_open,
		.Close          = ua3reo_dev_debug_ptt_key_if_close,
    .Received       = ua3reo_dev_debug_ptt_key_if_in_cmplt,
    .Transmitted    = ua3reo_dev_debug_ptt_key_if_out_cmplt,
};

USBD_CDC_IfHandleType _ua3reo_dev_debug_ptt_key_if = {
    .App = &ua3reo_dev_debug_ptt_key_app,
    .Base.AltCount = 1,
}, *const ua3reo_dev_debug_ptt_key_if = &_ua3reo_dev_debug_ptt_key_if;

static void ua3reo_dev_debug_ptt_key_if_open(void* itf, USBD_CDC_LineCodingType * lc)
{
  uint8_t buff[1];
	USBD_CDC_Receive(ua3reo_dev_debug_ptt_key_if, buff, 1);
}

static void ua3reo_dev_debug_ptt_key_if_close(void* itf)
{
    
}

static void ua3reo_dev_debug_ptt_key_if_in_cmplt(void* itf, uint8_t * pbuf, uint16_t length)
{
	uint8_t buff[length];
  USBD_CDC_Transmit(ua3reo_dev_debug_ptt_key_if, pbuf, length);
	USBD_CDC_Receive(ua3reo_dev_debug_ptt_key_if, buff, length);
}

static void ua3reo_dev_debug_ptt_key_if_out_cmplt(void* itf, uint8_t * pbuf, uint16_t length)
{
    //USBD_CDC_Receive(ua3reo_dev_debug_ptt_key_if,&pbuf, length);
}

void USBD_CDC_Transmit_FIFO(USBD_CDC_IfHandleType *itf, uint8_t *data, uint16_t length)
{
	if(length<=CDC_TX_FIFO_BUFFER_SIZE)
		for(uint16_t i=0;i<length;i++)
		{
			cdc_tx_fifo[cdc_tx_fifo_head]=data[i];
			cdc_tx_fifo_head++;
			if(cdc_tx_fifo_head>=CDC_TX_FIFO_BUFFER_SIZE)
			{
				cdc_tx_fifo_head=0;
				memset(&cdc_tx_fifo,0,CDC_TX_FIFO_BUFFER_SIZE);
			}
		}
}

void USBD_CDC_Transmit_FIFO_Events(USBD_CDC_IfHandleType *itf)
{
	uint8_t temp_buff[CDC_TX_FIFO_BUFFER_SIZE]={0};
	for(int i=0;i<CDC_TX_FIFO_BUFFER_SIZE;i++)
		temp_buff[i]=cdc_tx_fifo[i];
	USBD_ReturnType res=USBD_CDC_Transmit(itf, temp_buff, cdc_tx_fifo_head);
	if(res==USBD_E_OK)
	{
		cdc_tx_fifo_head=0;
		memset(&cdc_tx_fifo,0,CDC_TX_FIFO_BUFFER_SIZE);
	}
}
