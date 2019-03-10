/*
Library:						TFT 2.4" LCD - ili9341
Written by:					Mohamed Yaqoob (Not from scratch, but referring to many open source libraries)
Date written:				20/01/2018

Description:				This library makes use of the FSMC interface of the STM32 board to control a TFT LCD.
										The concept shown here is exactly the same for other TFT LCDs, might need to use 16 bits for
										some LCDs, but the method is similar.
										You can use this as a starting point to program your own LCD and share it with us ;)
*/

//Header files
#include "MA_ILI9341.h"
#include "../functions.h"

static uint8_t rotationNum = 1;
static bool _cp437 = false;

static uint16_t text_cursor_y = 0;
static uint16_t text_cursor_x = 0;
static bool wrap = false;

//***** Functions prototypes *****//
//1. Write Command to LCD
void ILI9341_SendCommand(uint16_t com)
{
	*(__IO uint16_t *)(0x60000000) = com;
}
uint16_t ILI9341_ReadCommand(void)
{
	return *(volatile uint16_t *)(0x60000000);
}

//2. Write data to LCD
void ILI9341_SendData(uint16_t data)
{
	*(__IO uint16_t *)(0x60080000) = data;
}

uint16_t ILI9341_ReadData(void)
{
	return *(__IO uint16_t *)(0x60080000);
}

//3. Set cursor position
void ILI9341_SetCursorAreaPosition(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {

	ILI9341_SendCommand(ILI9341_COLUMN_ADDR);
	ILI9341_SendData(x1 >> 8);
	ILI9341_SendData(x1 & 0xFF);
	ILI9341_SendData(x2 >> 8);
	ILI9341_SendData(x2 & 0xFF);
	ILI9341_SendCommand(ILI9341_PAGE_ADDR);
	ILI9341_SendData(y1 >> 8);
	ILI9341_SendData(y1 & 0xFF);
	ILI9341_SendData(y2 >> 8);
	ILI9341_SendData(y2 & 0xFF);
	ILI9341_SendCommand(ILI9341_GRAM);
}

void ILI9341_SetCursorPosition(uint16_t x, uint16_t y) {

	ILI9341_SendCommand(ILI9341_COLUMN_ADDR);
	ILI9341_SendData(x >> 8);
	ILI9341_SendData(x & 0xFF);
	ILI9341_SendData(x >> 8);
	ILI9341_SendData(x & 0xFF);
	ILI9341_SendCommand(ILI9341_PAGE_ADDR);
	ILI9341_SendData(y >> 8);
	ILI9341_SendData(y & 0xFF);
	ILI9341_SendData(y >> 8);
	ILI9341_SendData(y & 0xFF);
	ILI9341_SendCommand(ILI9341_GRAM);
	text_cursor_x=x;
	text_cursor_y=y;
}
uint16_t ILI9341_GetCurrentXOffset(void)
{
	return text_cursor_x;
}
//4. Initialise function
void ILI9341_Init(void)
{
	ILI9341_SendCommand(ILI9341_RESET); // software reset comand
	HAL_Delay(100);
	ILI9341_SendCommand(ILI9341_DISPLAY_OFF); // display off
	//------------power control------------------------------
	ILI9341_SendCommand(ILI9341_POWER1); // power control
	ILI9341_SendData(0x09); // GVDD = 3.3v
	ILI9341_SendCommand(ILI9341_POWER2); // power control
	ILI9341_SendData(0x10); // AVDD=VCIx2, VGH=VCIx7, VGL=-VCIx4
	//--------------VCOM-------------------------------------
	ILI9341_SendCommand(ILI9341_VCOM1); // vcom control
	ILI9341_SendData(0x18); // Set the VCOMH voltage (0x18 = 3.3v)
	ILI9341_SendData(0x3e); // Set the VCOML voltage (0x3E = -0.950v)
	ILI9341_SendCommand(ILI9341_VCOM2); // vcom control
	ILI9341_SendData(0xbe);
	//------------memory access control------------------------

	ILI9341_SendCommand(ILI9341_MAC); // memory access control
	ILI9341_SendData(0x48);

	ILI9341_SendCommand(ILI9341_PIXEL_FORMAT); // pixel format set
	ILI9341_SendData(0x55); // 16bit /pixel

	ILI9341_SendCommand(ILI9341_FRC);
	ILI9341_SendData(0);
	ILI9341_SendData(0x1F);

	//-------------ddram ----------------------------

	ILI9341_SendCommand(ILI9341_COLUMN_ADDR); // column set
	ILI9341_SendData(0x00); // x0_HIGH---0
	ILI9341_SendData(0x00); // x0_LOW----0
	ILI9341_SendData(0x00); // x1_HIGH---240
	ILI9341_SendData(0xEF); // x1_LOW----240
	ILI9341_SendCommand(ILI9341_PAGE_ADDR); // page address set
	ILI9341_SendData(0x00); // y0_HIGH---0
	ILI9341_SendData(0x00); // y0_LOW----0
	ILI9341_SendData(0x01); // y1_HIGH---320
	ILI9341_SendData(0x3F); // y1_LOW----320

	ILI9341_SendCommand(ILI9341_TEARING_OFF); // tearing effect off
	//LCD_write_cmd(ILI9341_TEARING_ON); // tearing effect on
	//LCD_write_cmd(ILI9341_DISPLAY_INVERSION); // display inversion
	ILI9341_SendCommand(ILI9341_Entry_Mode_Set); // entry mode set
	// Deep Standby Mode: OFF
	// Set the output level of gate driver G1-G320: Normal display
	// Low voltage detection: Disable
	ILI9341_SendData(0x07);

	//-----------------display------------------------

	ILI9341_SendCommand(ILI9341_DFC); // display function control
	//Set the scan mode in non-display area
	//Determine source/VCOM output in a non-display area in the partial display mode
	ILI9341_SendData(0x0a);
	//Select whether the liquid crystal type is normally white type or normally black type
	//Sets the direction of scan by the gate driver in the range determined by SCN and NL
	//Select the shift direction of outputs from the source driver
	//Sets the gate driver pin arrangement in combination with the GS bit to select the optimal scan mode for the module
	//Specify the scan cycle interval of gate driver in non-display area when PTG to select interval scan
	ILI9341_SendData(0x82);
	// Sets the number of lines to drive the LCD at an interval of 8 lines
	ILI9341_SendData(0x27);
	ILI9341_SendData(0x00); // clock divisor

	ILI9341_SendCommand(ILI9341_SLEEP_OUT); // sleep out
	HAL_Delay(100);
	ILI9341_SendCommand(ILI9341_DISPLAY_ON); // display on
	HAL_Delay(100);
	ILI9341_SendCommand(ILI9341_GRAM); // memory write
	HAL_Delay(5);
}

//5. Write data to a single pixel
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
	ILI9341_SetCursorPosition(x, y);
	ILI9341_SendData(color);
}

uint16_t ILI9341_ReadPixel(void) {
	ILI9341_SendCommand(0x00);
	ILI9341_SendCommand(ILI9341_CMD_MEMORY_READ);
	//ILI9341_SendData(0x00);
	//HAL_Delay(1);
	//return ILI9341_ReadCommand();
	return ILI9341_ReadData();
}

//6. Fill the entire screen with a background color
void ILI9341_Fill(uint16_t color) {
	uint32_t n = ILI9341_PIXEL_COUNT;

	if (rotationNum == 1 || rotationNum == 3)
	{
		ILI9341_SetCursorAreaPosition(0, 0, ILI9341_WIDTH - 1, ILI9341_HEIGHT - 1);
	}
	else if (rotationNum == 2 || rotationNum == 4)
	{
		ILI9341_SetCursorAreaPosition(0, 0, ILI9341_HEIGHT - 1, ILI9341_WIDTH - 1);
	}

	while (n) {
		n--;
		ILI9341_SendData(color);
	}
}
//7. Rectangle drawing functions
void ILI9341_Fill_RectXY(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, uint16_t color) {
	uint32_t n = ((x1 + 1) - x0)*((y1 + 1) - y0);
	if (n > ILI9341_PIXEL_COUNT) n = ILI9341_PIXEL_COUNT;
	ILI9341_SetCursorAreaPosition(x0, y0, x1, y1);
	while (n) {
		n--;
		ILI9341_SendData(color);
	}
}

void ILI9341_Fill_RectWH(unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint16_t color) {
	ILI9341_Fill_RectXY(x, y, x + w, y + h, color);
}

//8. Circle drawing functions
void ILI9341_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ILI9341_DrawPixel(x0, y0 + r, color);
	ILI9341_DrawPixel(x0, y0 - r, color);
	ILI9341_DrawPixel(x0 + r, y0, color);
	ILI9341_DrawPixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ILI9341_DrawPixel(x0 + x, y0 + y, color);
		ILI9341_DrawPixel(x0 - x, y0 + y, color);
		ILI9341_DrawPixel(x0 + x, y0 - y, color);
		ILI9341_DrawPixel(x0 - x, y0 - y, color);
		ILI9341_DrawPixel(x0 + y, y0 + x, color);
		ILI9341_DrawPixel(x0 - y, y0 + x, color);
		ILI9341_DrawPixel(x0 + y, y0 - x, color);
		ILI9341_DrawPixel(x0 - y, y0 - x, color);
	}
}

static void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if (cornername & 0x1) {
			ILI9341_drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			ILI9341_drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
		}
		if (cornername & 0x2) {
			ILI9341_drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
			ILI9341_drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
		}
	}
}
void ILI9341_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	ILI9341_drawFastVLine(x0, y0 - r, 2 * r + 1, color);
	fillCircleHelper(x0, y0, r, 3, 0, color);
}

//9. Line drawing functions
void ILI9341_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	}
	else {
		ystep = -1;
	}

	for (; x0 <= x1; x0++) {
		if (steep) {
			ILI9341_DrawPixel(y0, x0, color);
		}
		else {
			ILI9341_DrawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}

void ILI9341_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	int16_t x2 = x + w - 1;
	if (x2 < x)
		ILI9341_Fill_RectXY(x2, y, x, y, color);
	else
		ILI9341_Fill_RectXY(x, y, x2, y, color);
}

void ILI9341_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	int16_t y2 = y + h - 1;
	if (y2 < y)
		ILI9341_Fill_RectXY(x, y2, x, y, color);
	else
		ILI9341_Fill_RectXY(x, y, x, y2, color);
}

void ILI9341_drawRectXY(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, uint16_t color) {
	ILI9341_drawFastHLine(x0, y0, x1 - x0, color);
	ILI9341_drawFastHLine(x0, y1, x1 - x0, color);
	ILI9341_drawFastVLine(x0, y0, y1 - y0, color);
	ILI9341_drawFastVLine(x1, y0, y1 - y0, color);
}
//10. Triangle drawing
void ILI9341_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
	ILI9341_drawLine(x0, y0, x1, y1, color);
	ILI9341_drawLine(x1, y1, x2, y2, color);
	ILI9341_drawLine(x2, y2, x0, y0, color);
}
void ILI9341_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
	int16_t a, b, y, last;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1) {
		swap(y0, y1); swap(x0, x1);
	}
	if (y1 > y2) {
		swap(y2, y1); swap(x2, x1);
	}
	if (y0 > y1) {
		swap(y0, y1); swap(x0, x1);
	}

	if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
		a = b = x0;
		if (x1 < a)      a = x1;
		else if (x1 > b) b = x1;
		if (x2 < a)      a = x2;
		else if (x2 > b) b = x2;
		ILI9341_drawFastHLine(a, y0, b - a + 1, color);
		return;
	}

	int16_t
		dx01 = x1 - x0,
		dy01 = y1 - y0,
		dx02 = x2 - x0,
		dy02 = y2 - y0,
		dx12 = x2 - x1,
		dy12 = y2 - y1,
		sa = 0,
		sb = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if (y1 == y2) last = y1;   // Include y1 scanline
	else         last = y1 - 1; // Skip it

	for (y = y0; y <= last; y++) {
		a = x0 + sa / dy01;
		b = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;

		if (a > b) swap(a, b);
		ILI9341_drawFastHLine(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++) {
		a = x1 + sa / dy12;
		b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;

		if (a > b) swap(a, b);
		ILI9341_drawFastHLine(a, y, b - a + 1, color);
	}
}

//11. Text printing functions
void ILI9341_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size)
{
	uint8_t line;
	if ((x >= ILI9341_NORMAL_WIDTH) || // Clip right
		(y >= ILI9341_NORMAL_HEIGHT) || // Clip bottom
		((x + 6 * size - 1) < 0) || // Clip left
		((y + 8 * size - 1) < 0)) // Clip top
		return;
	
	if (!_cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior
	ILI9341_SetCursorAreaPosition(x,y,x+6*size-1,y+8*size-1); //char area
	
	for (int8_t j = 0; j < 8; j++) { //y line out
		for(int8_t s_y=0;s_y<size;s_y++) //y size scale
			for (int8_t i = 0; i < 6; i++) { //x line out
			{
				if (i == 5)
					line = 0x0;
				else
					line = pgm_read_byte(font1 + (c * 5) + i); //read font
				line >>= j;
				for(int8_t s_x=0;s_x<size;s_x++) //x size scale
				{
					if (line & 0x1) 
						ILI9341_SendData(color); //font pixel
					else
						ILI9341_SendData(bg); //background pixel
				}
			}
		}
	}
}

void ILI9341_printText(char text[], int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t size)
{
	int16_t offset;
	offset = size * 6;
	for (uint16_t i = 0; i < 40 && text[i] != NULL; i++)
	{
		ILI9341_drawChar(x + (offset*i), y, text[i], color, bg, size);
	}
}

void ILI9341_drawCharFont(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, GFXfont gfxFont)
{
	c -= (uint8_t)pgm_read_byte(&gfxFont.first);
	GFXglyph *glyph  = &(((GFXglyph *)pgm_read_pointer(&gfxFont.glyph))[c]);
	uint8_t  *bitmap = (uint8_t *)pgm_read_pointer(&gfxFont.bitmap);

	uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
	uint8_t  w  = pgm_read_byte(&glyph->width), h  = pgm_read_byte(&glyph->height), xa  = pgm_read_byte(&glyph->xAdvance);
	int8_t   xo = pgm_read_byte(&glyph->xOffset), yo = pgm_read_byte(&glyph->yOffset);
	uint8_t  xx, yy, bits = 0, bit = 0;

	ILI9341_SetCursorAreaPosition(x,y+yo,x+xa-1,y+yo+h-1); //char area
	
	for(yy=0; yy<h; yy++) {
		for(xx=0; xx<(xa); xx++) {
			if(xx<(xo) || xx>=(xo+w))
			{
				ILI9341_SendData(bg); //background pixel
				continue;
			}
			if(!(bit++ & 7)) {
				bits = pgm_read_byte(&bitmap[bo++]);
			}
			if(bits & 0x80) {
				ILI9341_SendData(color); //font pixel
			}
			else
			{
				ILI9341_SendData(bg); //background pixel
			}
			bits <<= 1;
		}
	}
}

void ILI9341_printTextFont(char text[], int16_t x, int16_t y, uint16_t color, uint16_t bg, GFXfont gfxFont)
{
	uint8_t c =0;
	text_cursor_x=x;
	text_cursor_y=y;
	for (uint16_t i = 0; i < 40 && text[i] != NULL; i++)
	{
		c = text[i];
		if(c == '\n') {
			text_cursor_x  = 0;
			text_cursor_y += (uint8_t)pgm_read_byte(&gfxFont.yAdvance);
		} else if(c != '\r') {
			uint8_t first = pgm_read_byte(&gfxFont.first);
			uint8_t last = pgm_read_byte(&gfxFont.last);
			if((c >= first) && (c <= last)) {
				GFXglyph *glyph = &(((GFXglyph *)pgm_read_pointer(&gfxFont.glyph))[c - first]);
				uint8_t w = pgm_read_byte(&glyph->width);
				uint8_t h = pgm_read_byte(&glyph->height);
				if((w > 0) && (h > 0)) { // Is there an associated bitmap?
					int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
					if(wrap && ((text_cursor_x + (xo + w)) > ILI9341_NORMAL_WIDTH)) {
						text_cursor_x  = 0;
						text_cursor_y += (uint8_t)pgm_read_byte(&gfxFont.yAdvance);
					}
					ILI9341_drawCharFont(text_cursor_x, text_cursor_y, c, color, bg, gfxFont);
				}
				text_cursor_x += (uint8_t)pgm_read_byte(&glyph->xAdvance);
			}
		}
	}
}

/**************************************************************************/
/*!
    @brief    Helper to determine size of a character with current font/size.
       Broke this out as it's used by both the PROGMEM- and RAM-resident getTextBounds() functions.
    @param    c     The ascii character in question
    @param    x     Pointer to x location of character
    @param    y     Pointer to y location of character
    @param    minx  Minimum clipping value for X
    @param    miny  Minimum clipping value for Y
    @param    maxx  Maximum clipping value for X
    @param    maxy  Maximum clipping value for Y
*/
/**************************************************************************/
void ILI9341_charBounds(char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy, GFXfont gfxFont) 
{
        if(c == '\n') { // Newline?
            *x  = 0;    // Reset x to zero, advance y by one line
            *y += (uint8_t)pgm_read_byte(&gfxFont.yAdvance);
        } else if(c != '\r') { // Not a carriage return; is normal char
            uint8_t first = pgm_read_byte(&gfxFont.first),
                    last  = pgm_read_byte(&gfxFont.last);
            if((c >= first) && (c <= last)) { // Char present in this font?
                GFXglyph *glyph = &(((GFXglyph *)pgm_read_pointer(
                  &gfxFont.glyph))[c - first]);
                uint8_t gw = pgm_read_byte(&glyph->width),
                        gh = pgm_read_byte(&glyph->height),
                        xa = pgm_read_byte(&glyph->xAdvance);
                int8_t  xo = pgm_read_byte(&glyph->xOffset),
                        yo = pgm_read_byte(&glyph->yOffset);
                if(wrap && ((*x+(((int16_t)xo+gw))) > ILI9341_NORMAL_WIDTH)) {
                    *x  = 0; // Reset x to zero, advance y by one line
                    *y += (uint8_t)pgm_read_byte(&gfxFont.yAdvance);
                }
                int16_t x1 = *x + xo,
                        y1 = *y + yo,
                        x2 = x1 + gw - 1,
                        y2 = y1 + gh - 1;
                if(x1 < *minx) *minx = x1;
                if(y1 < *miny) *miny = y1;
                if(x2 > *maxx) *maxx = x2;
                if(y2 > *maxy) *maxy = y2;
                *x += xa;
            }
        }
}

/**************************************************************************/
/*!
    @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
    @param    str     The ascii string to measure
    @param    x       The current cursor X
    @param    y       The current cursor Y
    @param    x1      The boundary X coordinate, set by function
    @param    y1      The boundary Y coordinate, set by function
    @param    w      The boundary width, set by function
    @param    h      The boundary height, set by function
*/
/**************************************************************************/
void ILI9341_getTextBounds(char text[], int16_t x, int16_t y, uint16_t *x1, uint16_t *y1, uint16_t *w, uint16_t *h, GFXfont gfxFont)
{
    uint8_t c; // Current character

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = ILI9341_NORMAL_WIDTH, miny = ILI9341_NORMAL_HEIGHT, maxx = -1, maxy = -1;

		for (uint16_t i = 0; i < 40 && text[i] != NULL; i++)
		{
			c = text[i];
      ILI9341_charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy, gfxFont);
		}

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}

//12. Image print (RGB 565, 2 bytes per pixel)
void ILI9341_printImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data, uint32_t size)
{
	uint32_t n = size;
	ILI9341_SetCursorAreaPosition(x, y, w + x - 1, h + y - 1);
	for (uint32_t i = 0; i < n; i++)
	{
		ILI9341_SendData(data[i]);
	}
}

//13. Set screen rotation
void ILI9341_setRotation(uint8_t rotate)
{
	switch (rotate)
	{
	case 1:
		rotationNum = 1;
		ILI9341_SendCommand(ILI9341_MEMCONTROL);
		ILI9341_SendData(ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
		break;
	case 2:
		rotationNum = 2;
		ILI9341_SendCommand(ILI9341_MEMCONTROL);
		ILI9341_SendData(ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
		break;
	case 3:
		rotationNum = 3;
		ILI9341_SendCommand(ILI9341_MEMCONTROL);
		ILI9341_SendData(ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
		break;
	case 4:
		rotationNum = 4;
		ILI9341_SendCommand(ILI9341_MEMCONTROL);
		ILI9341_SendData(ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
		break;
	default:
		rotationNum = 1;
		ILI9341_SendCommand(ILI9341_MEMCONTROL);
		ILI9341_SendData(ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
		break;
	}
}

uint16_t rgb888torgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	uint16_t b = (blue >> 3) & 0x1f;
	uint16_t g = ((green >> 2) & 0x3f) << 5;
	uint16_t r = ((red >> 3) & 0x1f) << 11;

	return (uint16_t)(r | g | b);
}

void ILI9341_vertScrollSetup(int16_t top, int16_t scrollines)
{
	ILI9341_SendCommand(0x33);
	ILI9341_SendData(top >> 8);
	ILI9341_SendData(top);
	ILI9341_SendData(scrollines >> 8);
	ILI9341_SendData(scrollines);
	ILI9341_SendData((320 - top - scrollines) >> 8);
	ILI9341_SendData((320 - top - scrollines));
}

void ILI9341_vertScroll(int16_t offset)
{
	ILI9341_SendCommand(0x37);
	ILI9341_SendData(offset >> 8);
	ILI9341_SendData(offset);
}
