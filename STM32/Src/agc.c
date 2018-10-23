#include "agc.h"
#include "stm32f4xx_hal.h"
#include "wm8731.h"
#include "math.h"
#include "arm_math.h"
#include "trx_manager.h"
#include "functions.h"
#include "settings.h"

uint16_t agc_wdsp_tau_decay[6] = { 4000,2000,500,250,50,500 };
agc_variables_t agc_wdsp;
bool agc_initialised = false;

void SetupAgcWdsp(void)
{
	float32_t tmp;
	float32_t sample_rate = WM8731_SampleMode * 1000;

	// this is a quick and dirty hack
	// it initialises the AGC variables once again,
	// if the decimation rate is changed
	// this should prevent confusion between the distance of in_index and out_index variables
	// because these are freshly initialised
	// in_index and out_index have a distance of 48 (sample rate 12000) or 96 (sample rate 24000)
	// so that has to be defined very well when filter from 4k8 to 5k0 (changing decimation rate from 4 to 2)

	// Start variables taken from wdsp
	// RXA.c !!!!
	/*
	0.001,                      // tau_attack
	0.250,                      // tau_decay
	4,                        // n_tau
	10000.0,                    // max_gain
	1.5,                      // var_gain
	1000.0,                     // fixed_gain
	1.0,                      // max_input
	1.0,                      // out_target
	0.250,                      // tau_fast_backaverage
	0.005,                      // tau_fast_decay
	5.0,                      // pop_ratio
	1,                        // hang_enable
	0.500,                      // tau_hang_backmult
	0.250,                      // hangtime
	0.250,                      // hang_thresh
	0.100);                     // tau_hang_decay
	 */
	 // one time initialization
	if (!agc_initialised)
	{

		/*
		 *
		 * 	//assign constants
	a->ring_buffsize = RB_SIZE;
	//do one-time initialization
	a->out_index = -1;
	a->ring_max = 0.0;
	a->volts = 0.0;
	a->save_volts = 0.0;
	a->fast_backaverage = 0.0;
	a->hang_backaverage = 0.0;
	a->hang_counter = 0;
	a->decay_type = 0;
	a->state = 0;
	a->ring = (double *)malloc0(RB_SIZE * sizeof(complex));
	a->abs_ring = (double *)malloc0(RB_SIZE * sizeof(double));
loadWcpAGC(a);
		 *
		 *
		 * */

		agc_wdsp.ring_buffsize = AGC_WDSP_RB_SIZE; //192; //96;
		//do one-time initialization
		agc_wdsp.out_index = -1; //agc_wdsp.ring_buffsize; // or -1 ??
		agc_wdsp.fixed_gain = 1.0;
		agc_wdsp.ring_max = 0.0;
		agc_wdsp.volts = 0.0;
		agc_wdsp.save_volts = 0.0;
		agc_wdsp.fast_backaverage = 0.0;
		agc_wdsp.hang_backaverage = 0.0;
		agc_wdsp.hang_counter = 0;
		agc_wdsp.decay_type = 0;
		agc_wdsp.state = 0;
		for (int idx = 0; idx < AGC_WDSP_RB_SIZE; idx++)
		{
			agc_wdsp.ring[idx * 2 + 0] = 0.0;
			agc_wdsp.ring[idx * 2 + 1] = 0.0;
			agc_wdsp.abs_ring[idx] = 0.0;
		}



		agc_wdsp.tau_attack = 0.001;               // tau_attack
		//    tau_decay = ts.agc_wdsp_tau_decay / 1000.0; // 0.250;                // tau_decay
		agc_wdsp.n_tau = 4;                        // n_tau

		//    max_gain = 1000.0; // 1000.0; determines the AGC threshold = knee level
		//  max_gain is powf (10.0, (float32_t)ts.agc_wdsp_thresh / 20.0);
		//    fixed_gain = ads.agc_rf_gain; //0.7; // if AGC == OFF, this gain is used
		agc_wdsp.max_input = (float32_t)ADC_CLIP_WARN_THRESHOLD; // which is 4096 at the moment
		//32767.0; // maximum value of 16-bit audio //  1.0; //
		agc_wdsp.out_targ = (float32_t)ADC_CLIP_WARN_THRESHOLD; // 4096, tweaked, so that volume when switching between the two AGCs remains equal
		//12000.0; // target value of audio after AGC
		agc_wdsp.tau_fast_backaverage = 0.250;    // tau_fast_backaverage
		agc_wdsp.tau_fast_decay = 0.005;          // tau_fast_decay
		agc_wdsp.pop_ratio = 5.0;                 // pop_ratio
		//    hang_enable = 0;                 // hang_enable
		agc_wdsp.tau_hang_backmult = 0.500;       // tau_hang_backmult

		agc_initialised = true;
	}
	//    var_gain = 32.0;  // slope of the AGC --> this is 10 * 10^(slope / 20) --> for 10dB slope, this is 30.0
	agc_wdsp.var_gain = powf((float32_t)10.0, (float32_t)agc_wdsp_slope / (float32_t)20.0 / (float32_t)10.0); // 10^(slope / 200)

	//    hangtime = 0.250;                // hangtime
	agc_wdsp.hangtime = (float32_t)agc_wdsp_hang_time / (float32_t)1000.0;
	//    hang_thresh = 0.250;             // hang_thresh

	//    tau_hang_decay = 0.100;          // tau_hang_decay

	//calculate internal parameters
	if (TRX.Agc)
	{
		switch (agc_wdsp_mode)
		{
		case 5: //agcOFF
			break;
		case 1: //agcLONG
			agc_wdsp.hangtime = 2.000;
			//      ts.agc_wdsp_tau_decay = 2000;
			//      hang_thresh = 1.0;
			//      ts.agc_wdsp_hang_enable = 1;
			break;
		case 2: //agcSLOW
			agc_wdsp.hangtime = 1.000;
			//      hang_thresh = 1.0;
			//      ts.agc_wdsp_tau_decay = 500;
			//      ts.agc_wdsp_hang_enable = 1;
			break;
		case 3: //agcMED
			//      hang_thresh = 1.0;
			agc_wdsp.hangtime = 0.250;
			//      ts.agc_wdsp_tau_decay = 250;
			break;
		case 4: //agcFAST
			//      hang_thresh = 1.0;
			agc_wdsp.hangtime = 0.100;
			//      ts.agc_wdsp_tau_decay = 50;
			break;
		case 0: //agcFrank --> very long
			//      ts.agc_wdsp_hang_enable = 0;
			//      hang_thresh = 0.300; // from which level on should hang be enabled
			agc_wdsp.hangtime = 3.000; // hang time, if enabled
			agc_wdsp.tau_hang_backmult = 0.500; // time constant exponential averager
			//      ts.agc_wdsp_tau_decay = 4000; // time constant decay long
			agc_wdsp.tau_fast_decay = 0.05;          // tau_fast_decay
			agc_wdsp.tau_fast_backaverage = 0.250; // time constant exponential averager
			break;
		default:
			break;
		}
		//ts.agc_wdsp_switch_mode = 0;
	}
	//  float32_t noise_offset = 10.0 * log10f(fhigh - rxa[channel].nbp0.p->flow)
	//          * size / rate);
	//  max_gain = out_target / var_gain * powf (10.0, (thresh + noise_offset) / 20.0));
	agc_wdsp.tau_hang_decay = (float32_t)agc_wdsp_tau_hang_decay / (float32_t)1000.0;
	agc_wdsp.tau_decay = (float32_t)agc_wdsp_tau_decay[agc_wdsp_mode] / (float32_t)1000.0;
	agc_wdsp.max_gain = powf((float32_t)10.0, (float32_t)agc_wdsp_thresh / (float32_t)20.0);
	agc_wdsp.fixed_gain = agc_wdsp.max_gain / (float32_t)10.0;
	// attack_buff_size is 48 for sample rate == 12000 and
	// 96 for sample rate == 24000
	agc_wdsp.attack_buffsize = (int)ceil(sample_rate * agc_wdsp.n_tau * agc_wdsp.tau_attack);

	agc_wdsp.in_index = agc_wdsp.attack_buffsize + agc_wdsp.out_index; // attack_buffsize + out_index can be more than 2x ring_bufsize !!!
	agc_wdsp.in_index %= agc_wdsp.ring_buffsize; // need to keep this within the index boundaries

	agc_wdsp.attack_mult = (float32_t)1.0 - expf((float32_t)-1.0 / (sample_rate * agc_wdsp.tau_attack));
	agc_wdsp.decay_mult = (float32_t)1.0 - expf((float32_t)-1.0 / (sample_rate * agc_wdsp.tau_decay));
	agc_wdsp.fast_decay_mult = (float32_t)1.0 - expf((float32_t)-1.0 / (sample_rate * agc_wdsp.tau_fast_decay));
	agc_wdsp.fast_backmult = (float32_t)1.0 - expf((float32_t)-1.0 / (sample_rate * agc_wdsp.tau_fast_backaverage));
	agc_wdsp.onemfast_backmult = (float32_t)1.0 - agc_wdsp.fast_backmult;

	agc_wdsp.out_target = agc_wdsp.out_targ * ((float32_t)1.0 - expf(-(float32_t)agc_wdsp.n_tau)) * (float32_t)0.9999;
	//  out_target = out_target * (1.0 - expf(-(float32_t)n_tau)) * 0.9999;
	agc_wdsp.min_volts = agc_wdsp.out_target / (agc_wdsp.var_gain * agc_wdsp.max_gain);
	agc_wdsp.inv_out_target = (float32_t)1.0 / agc_wdsp.out_target;

	tmp = log10f(agc_wdsp.out_target / (agc_wdsp.max_input * agc_wdsp.var_gain * agc_wdsp.max_gain));
	if (tmp == (float32_t)0.0)
	{
		tmp = (float32_t)1e-16;
	}
	agc_wdsp.slope_constant = (agc_wdsp.out_target * ((float32_t)1.0 - (float32_t)1.0 / agc_wdsp.var_gain)) / tmp;

	agc_wdsp.inv_max_input = (float32_t)1.0 / agc_wdsp.max_input;

	if (agc_wdsp.max_input > agc_wdsp.min_volts)
	{
		float32_t convert = powf((float32_t)10.0, (float32_t)agc_wdsp_hang_thresh / (float32_t)20.0);
		tmp = (convert - agc_wdsp.min_volts) / (agc_wdsp.max_input - agc_wdsp.min_volts);
		if (tmp < (float32_t)1e-8)
		{
			tmp = (float32_t)1e-8;
		}
		agc_wdsp.hang_thresh = (float32_t)1.0 + (float32_t)0.125 * log10f(tmp);
	}
	else
	{
		agc_wdsp.hang_thresh = (float32_t)1.0;
	}

	tmp = powf((float32_t)10.0, (agc_wdsp.hang_thresh - (float32_t)1.0) / (float32_t)0.125);
	agc_wdsp.hang_level = (agc_wdsp.max_input * tmp + (agc_wdsp.out_target /
		(agc_wdsp.var_gain * agc_wdsp.max_gain)) * ((float32_t)1.0 - tmp)) * (float32_t)0.637;

	agc_wdsp.hang_backmult = (float32_t)1.0 - expf((float32_t)-1.0 / (sample_rate * agc_wdsp.tau_hang_backmult));
	agc_wdsp.onemhang_backmult = (float32_t)1.0 - agc_wdsp.hang_backmult;

	agc_wdsp.hang_decay_mult = (float32_t)1.0 - expf((float32_t)-1.0 / (sample_rate * agc_wdsp.tau_hang_decay));
}

void RxAgcWdsp(int16_t blockSize, float32_t *agcbuffer1)
{
	// Be careful: the original source code has no comments,
	// all comments added by DD4WH, February 2017: comments could be wrong, misinterpreting or highly misleading!
	//
	if (!TRX.Agc)  // AGC OFF
	{
		for (uint16_t i = 0; i < blockSize; i++)
		{
			agcbuffer1[i] = agcbuffer1[i] * agc_wdsp.fixed_gain;
		}
		return;
	}

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

		if ((agc_wdsp.abs_out_sample >= agc_wdsp.ring_max) && (agc_wdsp.abs_out_sample > (float32_t)0.0))
		{
			agc_wdsp.ring_max = (float32_t)0.0;
			int k = agc_wdsp.out_index;

			for (uint16_t j = 0; j < agc_wdsp.attack_buffsize; j++)
			{
				if (++k == agc_wdsp.ring_buffsize)
				{
					k = 0;
				}
				if (agc_wdsp.abs_ring[k] > agc_wdsp.ring_max)
				{
					agc_wdsp.ring_max = agc_wdsp.abs_ring[k];
				}
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
						agc_wdsp.hang_counter = (int)(agc_wdsp.hangtime * WM8731_SampleMode * 1000);
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
		if (vo > (float32_t)0.0)
		{
			vo = (float32_t)0.0;
		}

		float32_t mult = (agc_wdsp.out_target - agc_wdsp.slope_constant * vo) / agc_wdsp.volts;
		agcbuffer1[i] = agc_wdsp.out_sample[0] * mult;
	}
	/*
if(ts.dmod_mode == DEMOD_AM || ts.dmod_mode == DEMOD_SAM)
{
	static float32_t    wold = 0.0;
	// eliminate DC in the audio after the AGC
	for(uint16_t i = 0; i < blockSize; i++)
	{
		float32_t w = agcbuffer1[i] + wold * 0.9999; // yes, I want a superb bass response ;-)
		agcbuffer1[i] = w - wold;
		wold = w;
	}
}
	*/
}
