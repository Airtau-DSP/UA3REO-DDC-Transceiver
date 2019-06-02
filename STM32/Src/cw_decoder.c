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

static float32_t sine = 0;
static float32_t cosine = 0;
static float32_t bw = 0;
static float32_t coeff = 0;
static float32_t Q1 = 0;
static float32_t Q2 = 0;
static float32_t  magnitude;
static int16_t magnitudelimit = 100;
static int16_t magnitudelimit_low = 100;
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
  k = (int) (0.5 + ((CWDECODER_SAMPLES * CWDECODER_TARGET_FREQ) / TRX_SAMPLERATE));
  omega = (2.0 * PI * k) / CWDECODER_SAMPLES;
  sine = arm_sin_f32(omega);
  cosine = arm_cos_f32(omega);
  coeff = 2.0 * cosine;
}

void CWDecoder_Process(float32_t* bufferIn)
{
	// The basic where we get the tone 
	for (uint8_t index = 0; index < CWDECODER_SAMPLES; index++){
	  float32_t Q0;
	  Q0 = coeff * Q1 - Q2 + (float32_t) bufferIn[index];
	  Q2 = Q1;
	  Q1 = Q0;	
  }
  float32_t magnitudeSquared = (Q1*Q1)+(Q2*Q2)-Q1*Q2*coeff;  // we do only need the real part //
  magnitude = sqrt(magnitudeSquared);
  Q2 = 0;
  Q1 = 0;
	
	// here we will try to set the magnitude limit automatic
	if (magnitude > magnitudelimit_low){
    magnitudelimit = (magnitudelimit +((magnitude - magnitudelimit)/6));  /// moving average filter
  }
  if (magnitudelimit < magnitudelimit_low)
	magnitudelimit = magnitudelimit_low;
	
	// now we check for the magnitude
	if(magnitude > magnitudelimit*0.6) // just to have some space up 
    realstate = true; 
  else
		realstate = false; 
	
	// here we clean up the state with a noise blanker
	if (realstate != realstatebefore){
		laststarttime = HAL_GetTick();
  }
  if ((HAL_GetTick()-laststarttime)> CWDECODER_NBTIME){
		if (realstate != filteredstate){
			filteredstate = realstate;
		}
  }
	
	// Then we do want to have some durations on high and low
	if (filteredstate != filteredstatebefore){
		if (filteredstate == true){
			starttimehigh = HAL_GetTick();
			lowduration = (HAL_GetTick() - startttimelow);
		}

		if (filteredstate == false){
			startttimelow = HAL_GetTick();
			highduration = (HAL_GetTick() - starttimehigh);
					if (highduration < (2*hightimesavg) || hightimesavg == 0){
				hightimesavg = (highduration+hightimesavg+hightimesavg)/3;     // now we know avg dit time ( rolling 3 avg)
			}
			if (highduration > (5*hightimesavg) ){
				hightimesavg = highduration+hightimesavg;     // if speed decrease fast ..
			}
		}
  }
	
	// now we will check which kind of baud we have - dit or dah
	// and what kind of pause we do have 1 - 3 or 7 pause 
	// we think that hightimeavg = 1 bit
	if (filteredstate != filteredstatebefore){
		stop = false;
		if (filteredstate == false){  //// we did end a HIGH
			if (highduration < (hightimesavg*2) && highduration > (hightimesavg*0.6)) /// 0.6 filter out false dits
			{ 
				strcat(code,".");
				sendToDebug_str(".");
			}
			if (highduration > (hightimesavg*2) && highduration < (hightimesavg*6))
			{ 
				strcat(code,"-");
				sendToDebug_str("-");
				wpm = (wpm + (1200/((highduration)/3)))/2;  //// the most precise we can do ;o)
			}
		}
	 
		if (filteredstate == true) //// we did end a LOW
		{  
			float32_t lacktime = 1.0f;
			if(wpm > 25)lacktime=1.0; ///  when high speeds we have to have a little more pause before new letter or new word 
			if(wpm > 30)lacktime=1.2;
			if(wpm > 35)lacktime=1.5;
		 
			if (lowduration > (hightimesavg*(2*lacktime)) && lowduration < hightimesavg*(5*lacktime)) // letter space
			{ 
				CWDecoder_Decode();
				code[0] = '\0';
				sendToDebug_str("/");
			}
			if (lowduration >= hightimesavg*(5*lacktime)) // word space
			{ 
				CWDecoder_Decode();
				code[0] = '\0';
				sendToDebug_str(" ");
				sendToDebug_newline();
		 }
		}
	}
	
	// write if no more letters
	if ((HAL_GetTick() - startttimelow) > (highduration * 6) && stop == false){
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
	/*
  if (strcmp(code,".-") == 0) printascii(65);
	if (strcmp(code,"-...") == 0) printascii(66);
	if (strcmp(code,"-.-.") == 0) printascii(67);
	if (strcmp(code,"-..") == 0) printascii(68);
	if (strcmp(code,".") == 0) printascii(69);
	if (strcmp(code,"..-.") == 0) printascii(70);
	if (strcmp(code,"--.") == 0) printascii(71);
	if (strcmp(code,"....") == 0) printascii(72);
	if (strcmp(code,"..") == 0) printascii(73);
	if (strcmp(code,".---") == 0) printascii(74);
	if (strcmp(code,"-.-") == 0) printascii(75);
	if (strcmp(code,".-..") == 0) printascii(76);
	if (strcmp(code,"--") == 0) printascii(77);
	if (strcmp(code,"-.") == 0) printascii(78);
	if (strcmp(code,"---") == 0) printascii(79);
	if (strcmp(code,".--.") == 0) printascii(80);
	if (strcmp(code,"--.-") == 0) printascii(81);
	if (strcmp(code,".-.") == 0) printascii(82);
	if (strcmp(code,"...") == 0) printascii(83);
	if (strcmp(code,"-") == 0) printascii(84);
	if (strcmp(code,"..-") == 0) printascii(85);
	if (strcmp(code,"...-") == 0) printascii(86);
	if (strcmp(code,".--") == 0) printascii(87);
	if (strcmp(code,"-..-") == 0) printascii(88);
	if (strcmp(code,"-.--") == 0) printascii(89);
	if (strcmp(code,"--..") == 0) printascii(90);

	if (strcmp(code,".----") == 0) printascii(49);
	if (strcmp(code,"..---") == 0) printascii(50);
	if (strcmp(code,"...--") == 0) printascii(51);
	if (strcmp(code,"....-") == 0) printascii(52);
	if (strcmp(code,".....") == 0) printascii(53);
	if (strcmp(code,"-....") == 0) printascii(54);
	if (strcmp(code,"--...") == 0) printascii(55);
	if (strcmp(code,"---..") == 0) printascii(56);
	if (strcmp(code,"----.") == 0) printascii(57);
	if (strcmp(code,"-----") == 0) printascii(48);

	if (strcmp(code,"..--..") == 0) printascii(63);
	if (strcmp(code,".-.-.-") == 0) printascii(46);
	if (strcmp(code,"--..--") == 0) printascii(44);
	if (strcmp(code,"-.-.--") == 0) printascii(33);
	if (strcmp(code,".--.-.") == 0) printascii(64);
	if (strcmp(code,"---...") == 0) printascii(58);
	if (strcmp(code,"-....-") == 0) printascii(45);
	if (strcmp(code,"-..-.") == 0) printascii(47);

	if (strcmp(code,"-.--.") == 0) printascii(40);
	if (strcmp(code,"-.--.-") == 0) printascii(41);
	if (strcmp(code,".-...") == 0) printascii(95);
	if (strcmp(code,"...-..-") == 0) printascii(36);
	if (strcmp(code,"...-.-") == 0) printascii(62);
	if (strcmp(code,".-.-.") == 0) printascii(60);
	if (strcmp(code,"...-.") == 0) printascii(126);
	//////////////////
	// The specials //
	//////////////////
	if (strcmp(code,".-.-") == 0) printascii(3);
	if (strcmp(code,"---.") == 0) printascii(4);
	if (strcmp(code,".--.-") == 0) printascii(6);
*/
}