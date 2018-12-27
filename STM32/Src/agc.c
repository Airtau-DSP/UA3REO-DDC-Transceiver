#include "agc.h"
#include "stm32f4xx_hal.h"
#include "wm8731.h"
#include "math.h"
#include "arm_math.h"
#include "trx_manager.h"
#include "functions.h"
#include "settings.h"
#include "profiler.h"

uint16_t agc_wdsp_tau_decay[6] = { 4000,2000,500,250,50,500 };
agc_variables_t agc_wdsp;
bool agc_initialised = false;

void SetupAgcWdsp(void)
{
	float32_t tmp;
	float32_t sample_rate = WM8731_SAMPLERATE;

	// this is a quick and dirty hack
	// it initialises the AGC variables once again,
	// if the decimation rate is changed
	// this should prevent confusion between the distance of in_index and out_index variables
	// because these are freshly initialised
	// in_index and out_index have a distance of 48 (sample rate 12000) or 96 (sample rate 24000)
	// so that has to be defined very well when filter from 4k8 to 5k0 (changing decimation rate from 4 to 2)

	 // one time initialization
	if (!agc_initialised)
	{
		agc_wdsp.ring_buffsize = AGC_WDSP_RB_SIZE; //192; //96;
		//do one-time initialization
		agc_wdsp.out_index = -1;
		agc_wdsp.ring_max = 0.0f;
		agc_wdsp.volts = 0.0f;
		agc_wdsp.save_volts = 0.0f;
		agc_wdsp.fast_backaverage = 0.0f;
		agc_wdsp.hang_backaverage = 0.0f;
		agc_wdsp.hang_counter = 0;
		agc_wdsp.decay_type = 0;
		agc_wdsp.state = 0;
		for (int idx = 0; idx < AGC_WDSP_RB_SIZE; idx++)
		{
			agc_wdsp.ring[idx * 2 + 0] = 0.0f;
			agc_wdsp.ring[idx * 2 + 1] = 0.0f;
			agc_wdsp.abs_ring[idx] = 0.0f;
		}
		agc_wdsp.tau_attack = 0.001f;               // tau_attack
		agc_wdsp.n_tau = 4;                        // n_tau
		agc_wdsp.max_input = ADC_CLIP_WARN_THRESHOLD; // which is 4096 at the moment
		agc_wdsp.out_targ = ADC_CLIP_WARN_THRESHOLD; // 4096, tweaked, so that volume when switching between the two AGCs remains equal
		agc_wdsp.tau_fast_backaverage = 0.250f;    // tau_fast_backaverage
		agc_wdsp.tau_fast_decay = 0.005f;          // tau_fast_decay
		agc_wdsp.pop_ratio = 5.0f;                 // pop_ratio
		agc_wdsp.tau_hang_backmult = 0.500f;       // tau_hang_backmult
		agc_initialised = true;
	}
	agc_wdsp.var_gain = powf(10.0f, agc_wdsp_slope / 20.0f / 10.0f); // 10^(slope / 200)
	agc_wdsp.hangtime = agc_wdsp_hang_time / 1000.0f;

	//calculate internal parameters
	if (TRX.Agc)
	{
		switch (agc_wdsp_mode)
		{
		case 5: //agcOFF
			break;
		case 1: //agcLONG
			agc_wdsp.hangtime = 2.000f;
			break;
		case 2: //agcSLOW
			agc_wdsp.hangtime = 1.000f;
			break;
		case 3: //agcMED
			agc_wdsp.hangtime = 0.250f;
			break;
		case 4: //agcFAST
			agc_wdsp.hangtime = 0.100f;
			break;
		case 0: //agcFrank --> very long
			agc_wdsp.hangtime = 3.000f; // hang time, if enabled
			agc_wdsp.tau_hang_backmult = 0.500f; // time constant exponential averager
			agc_wdsp.tau_fast_decay = 0.05f;          // tau_fast_decay
			agc_wdsp.tau_fast_backaverage = 0.250f; // time constant exponential averager
			break;
		default:
			break;
		}
	}
	agc_wdsp.tau_hang_decay = agc_wdsp_tau_hang_decay / 1000.0f;
	agc_wdsp.tau_decay = agc_wdsp_tau_decay[agc_wdsp_mode] / 1000.0f;
	agc_wdsp.max_gain = powf(10.0f, agc_wdsp_thresh / 20.0f);
	agc_wdsp.attack_buffsize = (int)ceil(sample_rate * agc_wdsp.n_tau * agc_wdsp.tau_attack);

	agc_wdsp.in_index = agc_wdsp.attack_buffsize + agc_wdsp.out_index; // attack_buffsize + out_index can be more than 2x ring_bufsize !!!
	agc_wdsp.in_index %= agc_wdsp.ring_buffsize; // need to keep this within the index boundaries

	agc_wdsp.attack_mult = 1.0f - expf(-1.0f / (sample_rate * agc_wdsp.tau_attack));
	agc_wdsp.decay_mult = 1.0f - expf(-1.0f / (sample_rate * agc_wdsp.tau_decay));
	agc_wdsp.fast_decay_mult = 1.0f - expf(-1.0f / (sample_rate * agc_wdsp.tau_fast_decay));
	agc_wdsp.fast_backmult = 1.0f - expf(-1.0f / (sample_rate * agc_wdsp.tau_fast_backaverage));
	agc_wdsp.onemfast_backmult = 1.0f - agc_wdsp.fast_backmult;

	agc_wdsp.out_target = agc_wdsp.out_targ * (1.0f - expf(-agc_wdsp.n_tau)) * 0.9999f;
	agc_wdsp.min_volts = agc_wdsp.out_target / (agc_wdsp.var_gain * agc_wdsp.max_gain);
	agc_wdsp.inv_out_target = 1.0f / agc_wdsp.out_target;

	tmp = log10f(agc_wdsp.out_target / (agc_wdsp.max_input * agc_wdsp.var_gain * agc_wdsp.max_gain));
	if (tmp == 0.0f)
	{
		tmp = 1e-16f;
	}
	agc_wdsp.slope_constant = (agc_wdsp.out_target * (1.0f - 1.0f / agc_wdsp.var_gain)) / tmp;

	agc_wdsp.inv_max_input = 1.0f / agc_wdsp.max_input;

	if (agc_wdsp.max_input > agc_wdsp.min_volts)
	{
		float32_t convert = powf(10.0f, agc_wdsp_hang_thresh / 20.0f);
		tmp = (convert - agc_wdsp.min_volts) / (agc_wdsp.max_input - agc_wdsp.min_volts);
		if (tmp < 1e-8f)
		{
			tmp = 1e-8f;
		}
		agc_wdsp.hang_thresh = 1.0f + 0.125f * log10f(tmp);
	}
	else
	{
		agc_wdsp.hang_thresh = 1.0f;
	}

	tmp = powf(10.0, (agc_wdsp.hang_thresh - 1.0f) / 0.125f);
	agc_wdsp.hang_level = (agc_wdsp.max_input * tmp + (agc_wdsp.out_target / (agc_wdsp.var_gain * agc_wdsp.max_gain)) * (1.0f - tmp)) * 0.637f;

	agc_wdsp.hang_backmult = 1.0f - expf(-1.0f / (sample_rate * agc_wdsp.tau_hang_backmult));
	agc_wdsp.onemhang_backmult = 1.0f - agc_wdsp.hang_backmult;

	agc_wdsp.hang_decay_mult = 1.0f - expf(-1.0f / (sample_rate * agc_wdsp.tau_hang_decay));
}

void RxAgcWdsp(int16_t blockSize, float32_t *agcbuffer1)
{
	// Be careful: the original source code has no comments,
	// all comments added by DD4WH, February 2017: comments could be wrong, misinterpreting or highly misleading!
	//
	for (uint16_t i = 0; i < blockSize; i++)
	{
		if (++agc_wdsp.out_index >= agc_wdsp.ring_buffsize)
		{
			agc_wdsp.out_index -= agc_wdsp.ring_buffsize;
		}
		if (++agc_wdsp.in_index >= agc_wdsp.ring_buffsize)
		{
			agc_wdsp.in_index -= agc_wdsp.ring_buffsize;
		}

		agc_wdsp.out_sample[0] = agc_wdsp.ring[2 * agc_wdsp.out_index];
		agc_wdsp.abs_out_sample = agc_wdsp.abs_ring[agc_wdsp.out_index];
		agc_wdsp.ring[2 * agc_wdsp.in_index] = agcbuffer1[i];
		agc_wdsp.abs_ring[agc_wdsp.in_index] = fabsf(agcbuffer1[i]);

		agc_wdsp.fast_backaverage = agc_wdsp.fast_backmult * agc_wdsp.abs_out_sample + agc_wdsp.onemfast_backmult * agc_wdsp.fast_backaverage;
		agc_wdsp.hang_backaverage = agc_wdsp.hang_backmult * agc_wdsp.abs_out_sample + agc_wdsp.onemhang_backmult * agc_wdsp.hang_backaverage;

		if (agc_wdsp.hang_backaverage > agc_wdsp.hang_level)
		{
			TRX_agc_wdsp_action = 1;
		}
		else
		{
			TRX_agc_wdsp_action = 0;
		}

		if ((agc_wdsp.abs_out_sample >= agc_wdsp.ring_max) && (agc_wdsp.abs_out_sample > 0.0f))
		{
			agc_wdsp.ring_max = 0.0f;
			int k = agc_wdsp.out_index;
			
			//for (uint16_t j = 0; j < agc_wdsp.attack_buffsize; j++)
			{
				if (++k == agc_wdsp.ring_buffsize) k = 0;
				if (agc_wdsp.abs_ring[k] > agc_wdsp.ring_max) agc_wdsp.ring_max = agc_wdsp.abs_ring[k];
			}
		}
		
		if (agc_wdsp.abs_ring[agc_wdsp.in_index] > agc_wdsp.ring_max)
		{
			agc_wdsp.ring_max = agc_wdsp.abs_ring[agc_wdsp.in_index];
		}

		if (agc_wdsp.hang_counter > 0)
		{
			--agc_wdsp.hang_counter;
		}
		
		switch (agc_wdsp.state)
		{
		case 0: // starting point after ATTACK
		{
			if (agc_wdsp.ring_max >= agc_wdsp.volts)
			{ // ATTACK
				agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.attack_mult;
			}
			else
			{ // DECAY
				if (agc_wdsp.volts > agc_wdsp.pop_ratio * agc_wdsp.fast_backaverage)
				{ // short time constant detector
					agc_wdsp.state = 1;
					agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.fast_decay_mult;
				}
				else
				{ // hang AGC enabled and being activated
									//1 - ts.agc_wdsp_hang_enable
					if (1 && (agc_wdsp.hang_backaverage > agc_wdsp.hang_level))
					{
						agc_wdsp.state = 2;
						agc_wdsp.hang_counter = (int)(agc_wdsp.hangtime * WM8731_SAMPLERATE);
						agc_wdsp.decay_type = 1;
					}
					else
					{// long time constant detector
						agc_wdsp.state = 3;
						agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.decay_mult;
						agc_wdsp.decay_type = 0;
					}
				}
			}
			break;
		}
		case 1: // short time constant decay
		{
			if (agc_wdsp.ring_max >= agc_wdsp.volts)
			{ // ATTACK
				agc_wdsp.state = 0;
				agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.attack_mult;
			}
			else
			{
				if (agc_wdsp.volts > agc_wdsp.save_volts)
				{// short time constant detector
					agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.fast_decay_mult;
				}
				else
				{
					if (agc_wdsp.hang_counter > 0)
					{
						agc_wdsp.state = 2;
					}
					else
					{
						if (agc_wdsp.decay_type == 0)
						{// long time constant detector
							agc_wdsp.state = 3;
							agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.decay_mult;
						}
						else
						{ // hang time constant
							agc_wdsp.state = 4;
							agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.hang_decay_mult;
						}
					}
				}
			}
			break;
		}
		case 2: // Hang is enabled and active, hang counter still counting
		{ // ATTACK
			if (agc_wdsp.ring_max >= agc_wdsp.volts)
			{
				agc_wdsp.state = 0;
				agc_wdsp.save_volts = agc_wdsp.volts;
				agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.attack_mult;
			}
			else
			{
				if (agc_wdsp.hang_counter == 0)
				{ // hang time constant
					agc_wdsp.state = 4;
					agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.hang_decay_mult;
				}
			}
			break;
		}
		case 3: // long time constant decay in progress
		{
			if (agc_wdsp.ring_max >= agc_wdsp.volts)
			{ // ATTACK
				agc_wdsp.state = 0;
				agc_wdsp.save_volts = agc_wdsp.volts;
				agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.attack_mult;
			}
			else
			{ // DECAY
				agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.decay_mult;
			}
			break;
		}
		case 4: // hang was enabled and counter has counted to zero --> hang decay
		{
			if (agc_wdsp.ring_max >= agc_wdsp.volts)
			{ // ATTACK
				agc_wdsp.state = 0;
				agc_wdsp.save_volts = agc_wdsp.volts;
				agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.attack_mult;
			}
			else
			{ // HANG DECAY
				agc_wdsp.volts += (agc_wdsp.ring_max - agc_wdsp.volts) * agc_wdsp.hang_decay_mult;
			}
			break;
		}
		}
		
		if (agc_wdsp.volts < agc_wdsp.min_volts)
		{
			agc_wdsp.volts = agc_wdsp.min_volts; // no AGC action is taking place
			TRX_agc_wdsp_action = 0;
		}
		else
		{
			// LED indicator for AGC action
			TRX_agc_wdsp_action = 1;
		}

		float32_t vo = log10f_fast(agc_wdsp.inv_max_input * agc_wdsp.volts);
		if (vo > 0.0f)
		{
			vo = 0.0f;
		}

		float32_t mult = (agc_wdsp.out_target - agc_wdsp.slope_constant * vo) / agc_wdsp.volts;
		agcbuffer1[i] = agc_wdsp.out_sample[0] * mult;
	}

	if (TRX_getMode() == TRX_MODE_AM)
	{
		static float32_t    wold = 0.0f;
		// eliminate DC in the audio after the AGC
		for (uint16_t i = 0; i < blockSize; i++)
		{
			float32_t w = agcbuffer1[i] + wold * 0.9999f; // yes, I want a superb bass response ;-)
			agcbuffer1[i] = w - wold;
			wold = w;
		}
	}
}
