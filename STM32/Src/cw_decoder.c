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
static int wpm = 0;
static char code[20] = { 0 };

static void CWDecoder_Decode(void);

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
				wpm = (wpm + (1200 / ((highduration) / 3))) / 2;  //// the most precise we can do ;o)
			}
		}

		if (filteredstate == true) //// we did end a LOW
		{
			float32_t lacktime = 1.0f;
			if (wpm > 25) lacktime = 1.0f; ///  when high speeds we have to have a little more pause before new letter or new word 
			if (wpm > 30) lacktime = 1.2f;
			if (wpm > 35) lacktime = 1.5f;

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
				sendToDebug_str(" ");
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
	LCD_UpdateQuery.StatusInfoGUI = true;
	realstatebefore = realstate;
	lasthighduration = highduration;
	filteredstatebefore = filteredstate;
}

static void CWDecoder_Decode(void)
{
	if (strcmp(code, ".-") == 0) sendToDebug_str("A");
	if (strcmp(code, "-...") == 0) sendToDebug_str("B");
	if (strcmp(code, "-.-.") == 0) sendToDebug_str("C");
	if (strcmp(code, "-..") == 0) sendToDebug_str("D");
	if (strcmp(code, ".") == 0) sendToDebug_str("E");
	if (strcmp(code, "..-.") == 0) sendToDebug_str("F");
	if (strcmp(code, "--.") == 0) sendToDebug_str("G");
	if (strcmp(code, "....") == 0) sendToDebug_str("H");
	if (strcmp(code, "..") == 0) sendToDebug_str("I");
	if (strcmp(code, ".---") == 0) sendToDebug_str("J");
	if (strcmp(code, "-.-") == 0) sendToDebug_str("K");
	if (strcmp(code, ".-..") == 0) sendToDebug_str("L");
	if (strcmp(code, "--") == 0) sendToDebug_str("M");
	if (strcmp(code, "-.") == 0) sendToDebug_str("N");
	if (strcmp(code, "---") == 0) sendToDebug_str("O");
	if (strcmp(code, ".--.") == 0) sendToDebug_str("P");
	if (strcmp(code, "--.-") == 0) sendToDebug_str("Q");
	if (strcmp(code, ".-.") == 0) sendToDebug_str("R");
	if (strcmp(code, "...") == 0) sendToDebug_str("S");
	if (strcmp(code, "-") == 0) sendToDebug_str("T");
	if (strcmp(code, "..-") == 0) sendToDebug_str("U");
	if (strcmp(code, "...-") == 0) sendToDebug_str("V");
	if (strcmp(code, ".--") == 0) sendToDebug_str("W");
	if (strcmp(code, "-..-") == 0) sendToDebug_str("X");
	if (strcmp(code, "-.--") == 0) sendToDebug_str("Y");
	if (strcmp(code, "--..") == 0) sendToDebug_str("Z");

	if (strcmp(code, ".----") == 0) sendToDebug_str("1");
	if (strcmp(code, "..---") == 0) sendToDebug_str("2");
	if (strcmp(code, "...--") == 0) sendToDebug_str("3");
	if (strcmp(code, "....-") == 0) sendToDebug_str("4");
	if (strcmp(code, ".....") == 0) sendToDebug_str("5");
	if (strcmp(code, "-....") == 0) sendToDebug_str("6");
	if (strcmp(code, "--...") == 0) sendToDebug_str("7");
	if (strcmp(code, "---..") == 0) sendToDebug_str("8");
	if (strcmp(code, "----.") == 0) sendToDebug_str("9");
	if (strcmp(code, "-----") == 0) sendToDebug_str("0");

	if (strcmp(code, "..--..") == 0) sendToDebug_str("?");
	if (strcmp(code, ".-.-.-") == 0) sendToDebug_str(".");
	if (strcmp(code, "--..--") == 0) sendToDebug_str(",");
	if (strcmp(code, "-.-.--") == 0) sendToDebug_str("!");
	if (strcmp(code, ".--.-.") == 0) sendToDebug_str("@");
	if (strcmp(code, "---...") == 0) sendToDebug_str(":");
	if (strcmp(code, "-....-") == 0) sendToDebug_str("-");
	if (strcmp(code, "-..-.") == 0) sendToDebug_str("/");

	if (strcmp(code, "-.--.") == 0) sendToDebug_str("(");
	if (strcmp(code, "-.--.-") == 0) sendToDebug_str(")");
	if (strcmp(code, ".-...") == 0) sendToDebug_str("_");
	if (strcmp(code, "...-..-") == 0) sendToDebug_str("$");
	if (strcmp(code, "...-.-") == 0) sendToDebug_str(">");
	if (strcmp(code, ".-.-.") == 0) sendToDebug_str("<");
	if (strcmp(code, "...-.") == 0) sendToDebug_str("~");
	//////////////////
	// The specials //
	//////////////////
	//if (strcmp(code,".-.-") == 0) printascii(3);
	//if (strcmp(code,"---.") == 0) printascii(4);
	//if (strcmp(code,".--.-") == 0) printascii(6);
}