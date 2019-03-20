#include <usbd_cdc.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#undef errno
extern int32_t errno;

/** @defgroup Templates */

/** @ingroup Templates
 * @defgroup ua3reo_dev_debug_ptt_key_if USB Debug serial interface
 * @{ */

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

