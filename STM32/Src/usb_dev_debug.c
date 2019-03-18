#include <usbd_cdc.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#undef errno
extern int32_t errno;

/** @defgroup Templates */

/** @ingroup Templates
 * @defgroup ua3reo_dev_debug_if USB Debug serial interface
 * @{ */

static void ua3reo_dev_debug_if_open         (void* itf, USBD_CDC_LineCodingType * lc);
static void ua3reo_dev_debug_if_in_cmplt     (void* itf, uint8_t * pbuf, uint16_t length);
static void ua3reo_dev_debug_if_out_cmplt    (void* itf, uint8_t * pbuf, uint16_t length);

static const USBD_CDC_AppType ua3reo_dev_debug_app =
{
    .Name           = "Debug serial port as standard I/O",
    .Open           = ua3reo_dev_debug_if_open,
    .Received       = ua3reo_dev_debug_if_in_cmplt,
    .Transmitted    = ua3reo_dev_debug_if_out_cmplt,
};

USBD_CDC_IfHandleType _ua3reo_dev_debug_if = {
    .App = &ua3reo_dev_debug_app,
    .Base.AltCount = 1,
}, *const ua3reo_dev_debug_if = &_ua3reo_dev_debug_if;

static void ua3reo_dev_debug_if_open(void* itf, USBD_CDC_LineCodingType * lc)
{
    
}

static void ua3reo_dev_debug_if_in_cmplt(void* itf, uint8_t * pbuf, uint16_t length)
{
	uint8_t buff[length];
	USBD_CDC_Receive(ua3reo_dev_debug_if, buff, length);
  USBD_CDC_Transmit(ua3reo_dev_debug_if, buff, length);
}

static void ua3reo_dev_debug_if_out_cmplt(void* itf, uint8_t * pbuf, uint16_t length)
{
    //USBD_CDC_Receive(ua3reo_dev_debug_if,&pbuf, length);
}

