#include "cw_decoder.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include "settings.h"
#include "functions.h"
#include "lcd.h"
#include "fpga.h"

volatile uint16_t CW_Decoder_WPM = 0;
char CW_Decoder_Text[CWDECODER_STRLEN] = "               ";

static float32_t sine = 0;
static float32_t cosine = 0;
static float32_t bw = 0;
static float32_t coeff = 0;
static float32_t Q1 = 0;
static float32_t Q2 = 0;
static float32_t magnitude = 0;
static int16_t magnitudelimit = 50;
static int16_t magnitudelimit_low = 50;
static bool realstate = false;
static bool realstatebefore = false;
static bool filteredstate = false;
static bool filteredstatebefore = false;
static bool stop = false;
static long laststarttime = 0;
static long starttimehigh = 0;
static long highduration = 0;
static long lowtimesavg = 0;
static long startttimelow = 0;
static long lowduration = 0;
static long hightimesavg = 0;
static long lasthighduration = 0;
static char code[20] = { 0 };

static void CWDecoder_Decode(void);
static void CWDecoder_PrintChar(char * str);

void CWDecoder_Init(void)
{
	//Алгоритм Гёрцеля (goertzel calculation)
	int16_t	k = 0;
	float32_t	omega = 0;
	bw = (TRX_SAMPLERATE / CWDECODER_SAMPLES);
	k = (int)(0.5 + ((CWDECODER_SAMPLES * CWDECODER_TARGET_FREQ) / TRX_SAMPLERATE));
	omega = (2.0 * PI * k) / CWDECODER_SAMPLES;
	sine = arm_sin_f32(omega);
	cosine = arm_cos_f32(omega);
	coeff = 2.0 * cosine;
}

void CWDecoder_Process(float32_t* bufferIn)
{
	// The basic where we get the tone 
	for (uint16_t index = 0; index < CWDECODER_SAMPLES; index++) {
		float32_t Q0;
		Q0 = coeff * Q1 - Q2 + (float32_t)bufferIn[index];
		Q2 = Q1;
		Q1 = Q0;
	}
	float32_t magnitudeSquared = (Q1*Q1) + (Q2*Q2) - Q1 * Q2*coeff;  // we do only need the real part //
	magnitude = sqrt(magnitudeSquared);
	Q2 = 0;
	Q1 = 0;

	// here we will try to set the magnitude limit automatic
	if (magnitude > magnitudelimit_low) {
		magnitudelimit = (magnitudelimit + ((magnitude - magnitudelimit) / CWDECODER_HIGH_AVERAGE));  /// moving average filter high
	}
	if (magnitudelimit < magnitudelimit_low)
		magnitudelimit = magnitudelimit_low;
	//sendToDebug_float32(magnitude,false);

	// now we check for the magnitude
	if ((magnitude > magnitudelimit*0.8) && (magnitude > magnitudelimit_low*1.2)) // just to have some space up 
		realstate = true;
	else
		realstate = false;

	// here we clean up the state with a noise blanker
	if (realstate != realstatebefore) {
		laststarttime = HAL_GetTick();
	}
	if ((HAL_GetTick() - laststarttime) > CWDECODER_NBTIME) {
		if (realstate != filteredstate) {
			filteredstate = realstate;
		}
	}
	//sendToDebug_uint8(filteredstate,true);

	// Then we do want to have some durations on high and low
	if (filteredstate != filteredstatebefore) {
		if (filteredstate == true) {
			starttimehigh = HAL_GetTick();
			lowduration = (HAL_GetTick() - startttimelow);
		}

		if (filteredstate == false) {
			startttimelow = HAL_GetTick();
			highduration = (HAL_GetTick() - starttimehigh);
			if (highduration < (2 * hightimesavg) || hightimesavg == 0) {
				hightimesavg = (highduration + hightimesavg + hightimesavg) / 3;     // now we know avg dit time ( rolling 3 avg)
			}
			if (highduration > (5 * hightimesavg)) {
				hightimesavg = highduration + hightimesavg;     // if speed decrease fast ..
			}
		}
	}

	// now we will check which kind of baud we have - dit or dah
	// and what kind of pause we do have 1 - 3 or 7 pause 
	// we think that hightimeavg = 1 bit
	if (filteredstate != filteredstatebefore) {
		stop = false;
		if (filteredstate == false) {  //// we did end a HIGH
			if (highduration < (hightimesavg * 2) && highduration >(hightimesavg*0.6)) /// 0.6 filter out false dits
			{
				strcat(code, ".");
				//sendToDebug_str(".");
			}
			if (highduration > (hightimesavg * 2) && highduration < (hightimesavg * 6))
			{
				strcat(code, "-");
				//sendToDebug_str("-");
				CW_Decoder_WPM = (CW_Decoder_WPM + (1200 / ((highduration) / 3))) / 2;  //// the most precise we can do ;o)
			}
		}

		if (filteredstate == true) //// we did end a LOW
		{
			float32_t lacktime = 1.0f;
			if (CW_Decoder_WPM > 25) lacktime = 1.0f; ///  when high speeds we have to have a little more pause before new letter or new word 
			if (CW_Decoder_WPM > 30) lacktime = 1.2f;
			if (CW_Decoder_WPM > 35) lacktime = 1.5f;

			if (lowduration > (hightimesavg*(2 * lacktime)) && lowduration < hightimesavg*(5 * lacktime)) // letter space
			{
				CWDecoder_Decode();
				code[0] = '\0';
				//sendToDebug_str("/");
			}
			if (lowduration >= hightimesavg * (5 * lacktime)) // word space
			{
				CWDecoder_Decode();
				code[0] = '\0';
				CWDecoder_PrintChar(" ");
				//sendToDebug_str(" ");
				//sendToDebug_newline();
			}
		}
	}

	// write if no more letters
	if ((HAL_GetTick() - startttimelow) > (highduration * 6) && stop == false) {
		CWDecoder_Decode();
		code[0] = '\0';
		stop = true;
	}

	// the end of main loop clean up
	realstatebefore = realstate;
	lasthighduration = highduration;
	filteredstatebefore = filteredstate;
}

static void CWDecoder_Decode(void)
{
	if (strcmp(code, ".-") == 0) CWDecoder_PrintChar("A");
	else if (strcmp(code, "-...") == 0) CWDecoder_PrintChar("B");
	else if (strcmp(code, "-.-.") == 0) CWDecoder_PrintChar("C");
	else if (strcmp(code, "-..") == 0) CWDecoder_PrintChar("D");
	else if (strcmp(code, ".") == 0) CWDecoder_PrintChar("E");
	else if (strcmp(code, "..-.") == 0) CWDecoder_PrintChar("F");
	else if (strcmp(code, "--.") == 0) CWDecoder_PrintChar("G");
	else if (strcmp(code, "....") == 0) CWDecoder_PrintChar("H");
	else if (strcmp(code, "..") == 0) CWDecoder_PrintChar("I");
	else if (strcmp(code, ".---") == 0) CWDecoder_PrintChar("J");
	else if (strcmp(code, "-.-") == 0) CWDecoder_PrintChar("K");
	else if (strcmp(code, ".-..") == 0) CWDecoder_PrintChar("L");
	else if (strcmp(code, "--") == 0) CWDecoder_PrintChar("M");
	else if (strcmp(code, "-.") == 0) CWDecoder_PrintChar("N");
	else if (strcmp(code, "---") == 0) CWDecoder_PrintChar("O");
	else if (strcmp(code, ".--.") == 0) CWDecoder_PrintChar("P");
	else if (strcmp(code, "--.-") == 0) CWDecoder_PrintChar("Q");
	else if (strcmp(code, ".-.") == 0) CWDecoder_PrintChar("R");
	else if (strcmp(code, "...") == 0) CWDecoder_PrintChar("S");
	else if (strcmp(code, "-") == 0) CWDecoder_PrintChar("T");
	else if (strcmp(code, "..-") == 0) CWDecoder_PrintChar("U");
	else if (strcmp(code, "...-") == 0) CWDecoder_PrintChar("V");
	else if (strcmp(code, ".--") == 0) CWDecoder_PrintChar("W");
	else if (strcmp(code, "-..-") == 0) CWDecoder_PrintChar("X");
	else if (strcmp(code, "-.--") == 0) CWDecoder_PrintChar("Y");
	else if (strcmp(code, "--..") == 0) CWDecoder_PrintChar("Z");

	else if (strcmp(code, ".----") == 0) CWDecoder_PrintChar("1");
	else if (strcmp(code, "..---") == 0) CWDecoder_PrintChar("2");
	else if (strcmp(code, "...--") == 0) CWDecoder_PrintChar("3");
	else if (strcmp(code, "....-") == 0) CWDecoder_PrintChar("4");
	else if (strcmp(code, ".....") == 0) CWDecoder_PrintChar("5");
	else if (strcmp(code, "-....") == 0) CWDecoder_PrintChar("6");
	else if (strcmp(code, "--...") == 0) CWDecoder_PrintChar("7");
	else if (strcmp(code, "---..") == 0) CWDecoder_PrintChar("8");
	else if (strcmp(code, "----.") == 0) CWDecoder_PrintChar("9");
	else if (strcmp(code, "-----") == 0) CWDecoder_PrintChar("0");

	else if (strcmp(code, "..--..") == 0) CWDecoder_PrintChar("?");
	else if (strcmp(code, ".-.-.-") == 0) CWDecoder_PrintChar(".");
	else if (strcmp(code, "--..--") == 0) CWDecoder_PrintChar(",");
	else if (strcmp(code, "-.-.--") == 0) CWDecoder_PrintChar("!");
	else if (strcmp(code, ".--.-.") == 0) CWDecoder_PrintChar("@");
	else if (strcmp(code, "---...") == 0) CWDecoder_PrintChar(":");
	else if (strcmp(code, "-....-") == 0) CWDecoder_PrintChar("-");
	else if (strcmp(code, "-..-.") == 0) CWDecoder_PrintChar("/");

	else if (strcmp(code, "-.--.") == 0) CWDecoder_PrintChar("(");
	else if (strcmp(code, "-.--.-") == 0) CWDecoder_PrintChar(")");
	else if (strcmp(code, ".-...") == 0) CWDecoder_PrintChar("_");
	else if (strcmp(code, "...-..-") == 0) CWDecoder_PrintChar("$");
	else if (strcmp(code, "...-.-") == 0) CWDecoder_PrintChar(">");
	else if (strcmp(code, ".-.-.") == 0) CWDecoder_PrintChar("<");
	else if (strcmp(code, "...-.") == 0) CWDecoder_PrintChar("~");
	//////////////////
	// The specials //
	//////////////////
	//else if (strcmp(code,".-.-") == 0) CWDecoder_PrintChar(""); ascii(3)
	//else if (strcmp(code,"---.") == 0) CWDecoder_PrintChar(""); ascii(4)
	//else if (strcmp(code,".--.-") == 0) CWDecoder_PrintChar(""); ascii(6)
}

static void CWDecoder_PrintChar(char * str)
{
	if(strlen(CW_Decoder_Text)>=CWDECODER_STRLEN)
		shiftTextLeft(CW_Decoder_Text, 1);
	strcat(CW_Decoder_Text, str);
	LCD_UpdateQuery.TextBar = true;
}
