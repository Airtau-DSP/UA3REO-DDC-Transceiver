#include "helper.h"
#include "stm32f4xx_hal.h"
#include "string.h"
#include <stdbool.h>
#include "settings.h"
#include "trx_manager.h"
#include "functions.h"

bool HELPER_ATT = false;
bool HELPER_PREAMP = false;
bool HELPER_AMP = false;
bool HELPER_AMP_POWER = false;
uint8_t HELPER_BPF = 0;

uint8_t HELPER_getBPFfromFreq(uint32_t freq)
{
	uint8_t res = 0;
	if (freq >= 1800000 && freq <= 2000000) res = 160;
	if (freq >= 3500000 && freq <= 3800000) res = 80;
	if (freq >= 6950000 && freq <= 7250000) res = 40;
	if (freq >= 8700000 && freq <= 9000000) res = 30;
	if (freq >= 13600000 && freq <= 14800000) res = 20;
	if (freq >= 17500000 && freq <= 19000000) res = 17;
	if (freq >= 20000000 && freq <= 22000000) res = 15;
	if (freq >= 23500000 && freq <= 26500000) res = 12;
	if (freq >= 26500000 && freq <= 30000000) res = 10;
	return res;
}

void HELPER_updateSettings(void)
{
	if (TRX.BPF)
		HELPER_setBPF(HELPER_getBPFfromFreq(TRX.Freq));
	else
		HELPER_setBPF(0);
	HELPER_setATT(TRX.Att);
	HELPER_setPREAMP(TRX.Preamp_HF);
	//HELPER_setAMP_POWER(TRX_ptt || TRX_tune);
	HELPER_setAMP(TRX_ptt || TRX_tune);
}

void HELPER_setATT(bool val)
{
	if (HELPER_ATT != val)
	{
		HELPER_ATT = val;
		if (val) HELPER_WriteCommand("ATT_ON");
		else HELPER_WriteCommand("ATT_OFF");
	}
}

void HELPER_setPREAMP(bool val)
{
	if (HELPER_PREAMP != val)
	{
		HELPER_PREAMP = val;
		if (val) HELPER_WriteCommand("PREAMP_ON");
		else HELPER_WriteCommand("PREAMP_OFF");
	}
}

void HELPER_setAMP_POWER(bool val)
{
	if (HELPER_AMP_POWER != val)
	{
		HELPER_AMP_POWER = val;
		if (val) HELPER_WriteCommand("AMP_POWER_ON");
		else HELPER_WriteCommand("AMP_POWER_OFF");
	}
}

void HELPER_setAMP(bool val)
{
	if (HELPER_AMP != val)
	{
		HELPER_AMP = val;
		if (val) HELPER_WriteCommand("AMP_ON");
		else HELPER_WriteCommand("AMP_OFF");
	}
}

void HELPER_setBPF(uint8_t val)
{
	if (HELPER_BPF != val)
	{
		HELPER_BPF = val;
		if (val == 0) HELPER_WriteCommand("DPF_NONE");
		if (val == 160) HELPER_WriteCommand("DPF_160");
		if (val == 80) HELPER_WriteCommand("DPF_80");
		if (val == 40) HELPER_WriteCommand("DPF_40");
		if (val == 30) HELPER_WriteCommand("DPF_30");
		if (val == 20) HELPER_WriteCommand("DPF_20");
		if (val == 17) HELPER_WriteCommand("DPF_17");
		if (val == 15) HELPER_WriteCommand("DPF_15");
		if (val == 12) HELPER_WriteCommand("DPF_12");
		if (val == 10) HELPER_WriteCommand("DPF_10");
	}
}

void HELPER_WriteCommand(char* data)
{
	if (HELPER_ENABLED)
	{
		char* eol = "\r\n";
		HAL_UART_Transmit(&huart6, (uint8_t*)data, strlen(data), 100);
		HAL_UART_Transmit(&huart6, (uint8_t*)eol, strlen(eol), 100);
	}
}
