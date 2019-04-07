#include "bands.h"
#include "functions.h"

const BAND_MAP BANDS[] =
{
	//160METERS
	{
		.name = "160m",
		.startFreq = 1810000,
		.endFreq = 2000000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 1810000,
				.endFreq = 1838000,
				.mode = TRX_MODE_CW_L
			},
			{
				.startFreq = 1838000,
				.endFreq = 1843000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 1843000,
				.endFreq = 2000000,
				.mode = TRX_MODE_LSB
			},
		},
		.regionsCount = 3,
	},
	//80METERS
	{
		.name = "80m",
		.startFreq = 3500000,
		.endFreq = 3800000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 3500000,
				.endFreq = 3580000,
				.mode = TRX_MODE_CW_L
			},
			{
				.startFreq = 3580000,
				.endFreq = 3600000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 3600000,
				.endFreq = 3800000,
				.mode = TRX_MODE_LSB
			},
		},
		.regionsCount = 3,
	},
	//40METERS
	{
		.name = "40m",
		.startFreq = 7000000,
		.endFreq = 7300000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 7000000,
				.endFreq = 7040000,
				.mode = TRX_MODE_CW_L
			},
			{
				.startFreq = 7040000,
				.endFreq = 7060000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 7060000,
				.endFreq = 7074000,
				.mode = TRX_MODE_LSB
			},
			{
				.startFreq = 7074000,
				.endFreq = 7080000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 7080000,
				.endFreq = 7200000,
				.mode = TRX_MODE_LSB
			},
			{
				.startFreq = 7200000,
				.endFreq = 7300000,
				.mode = TRX_MODE_AM
			},
		},
		.regionsCount = 6,
	},
	//30METERS
	{
		.name = "30m",
		.startFreq = 10100000,
		.endFreq = 10150000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 10100000,
				.endFreq = 10140000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 10140000,
				.endFreq = 10150000,
				.mode = TRX_MODE_DIGI_U
			},
		},
		.regionsCount = 2,
	},
	//20METERS
	{
		.name = "20m",
		.startFreq = 14000000,
		.endFreq = 14350000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 14000000,
				.endFreq = 14070000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 14070000,
				.endFreq = 14112000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 14112000,
				.endFreq = 14350000,
				.mode = TRX_MODE_USB
			},
		},
		.regionsCount = 3,
	},
	//17METERS
	{
		.name = "17m",
		.startFreq = 18068000,
		.endFreq = 18168000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 18068000,
				.endFreq = 18095000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 18095000,
				.endFreq = 18109000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 18109000,
				.endFreq = 18168000,
				.mode = TRX_MODE_USB
			},
		},
		.regionsCount = 3,
	},
	//15METERS
	{
		.name = "15m",
		.startFreq = 21000000,
		.endFreq = 21450000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 21000000,
				.endFreq = 21070000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 21070000,
				.endFreq = 21149000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 21149000,
				.endFreq = 21450000,
				.mode = TRX_MODE_USB
			},
		},
		.regionsCount = 4,
	},
	//12METERS
	{
		.name = "12m",
		.startFreq = 24890000,
		.endFreq = 24990000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 24890000,
				.endFreq = 24915000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 24915000,
				.endFreq = 24940000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 24940000,
				.endFreq = 24990000,
				.mode = TRX_MODE_USB
			},
		},
		.regionsCount = 4,
	},
	//10METERS
	{
		.name = "10m",
		.startFreq = 28000000,
		.endFreq = 29700000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 28000000,
				.endFreq = 28070000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 28070000,
				.endFreq = 28190000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 28190000,
				.endFreq = 28199000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 28199000,
				.endFreq = 28300000,
				.mode = TRX_MODE_USB
			},
			{
				.startFreq = 28300000,
				.endFreq = 28320000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 28320000,
				.endFreq = 29200000,
				.mode = TRX_MODE_USB
			},
			{
				.startFreq = 29200000,
				.endFreq = 29300000,
				.mode = TRX_MODE_DIGI_U
			},
			{
				.startFreq = 29300000,
				.endFreq = 29520000,
				.mode = TRX_MODE_USB
			},
			{
				.startFreq = 29520000,
				.endFreq = 29700000,
				.mode = TRX_MODE_NFM
			},
		},
		.regionsCount = 9,
	},
	//FM RADIO 1
	{
		.name = "FM1",
		.startFreq = 65900000,
		.endFreq = 74000000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 65900000,
				.endFreq = 74000000,
				.mode = TRX_MODE_WFM
			},
		},
		.regionsCount = 1,
	},
	//FM RADIO 2
	{
		.name = "FM2",
		.startFreq = 87500000,
		.endFreq = 108000000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 87500000,
				.endFreq = 108000000,
				.mode = TRX_MODE_WFM
			},
		},
		.regionsCount = 1,
	},
	//2 meter
	{
		.name = "VHF",
		.startFreq = 144000000,
		.endFreq = 146000000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 144000000,
				.endFreq = 144025000,
				.mode = TRX_MODE_NFM
			},
			{
				.startFreq = 144025000,
				.endFreq = 144110000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 144110000,
				.endFreq = 144491000,
				.mode = TRX_MODE_USB
			},
			{
				.startFreq = 144491000,
				.endFreq = 146000000,
				.mode = TRX_MODE_NFM
			},
		},
		.regionsCount = 4,
	},
	//70cm
	{
		.name = "UHF",
		.startFreq = 430000000,
		.endFreq = 440000000,
		.regions = (const REGION_MAP[])
		{
			{
				.startFreq = 430000000,
				.endFreq = 432000000,
				.mode = TRX_MODE_NFM
			},
			{
				.startFreq = 432000000,
				.endFreq = 432100000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 432100000,
				.endFreq = 432400000,
				.mode = TRX_MODE_USB
			},
			{
				.startFreq = 432400000,
				.endFreq = 432500000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 432500000,
				.endFreq = 434000000,
				.mode = TRX_MODE_NFM
			},
			{
				.startFreq = 434000000,
				.endFreq = 434100000,
				.mode = TRX_MODE_CW_U
			},
			{
				.startFreq = 434100000,
				.endFreq = 440000000,
				.mode = TRX_MODE_NFM
			},
		},
		.regionsCount = 7,
	},
	//
};

int8_t getBandFromFreq(uint32_t freq)
{
	for (int b = 0; b < BANDS_COUNT; b++)
		if (BANDS[b].startFreq <= freq && freq <= BANDS[b].endFreq)
			return b;
	return -1;
}

uint8_t getModeFromFreq(uint32_t freq)
{
	uint8_t ret = 0;
	if (freq < 10000000) ret = TRX_MODE_LSB;
	if (freq > 10000000) ret = TRX_MODE_USB;
	if (freq > 30000000) ret = TRX_MODE_NFM;
	for (int b = 0; b < BANDS_COUNT; b++)
	{
		if (BANDS[b].startFreq <= freq && freq <= BANDS[b].endFreq)
			for (int r = 0; r < BANDS[b].regionsCount; r++)
			{
				if (BANDS[b].regions[r].startFreq <= freq && freq < BANDS[b].regions[r].endFreq)
				{
					ret = BANDS[b].regions[r].mode;
					return ret;
				}
			}
	}
	return ret;
}
