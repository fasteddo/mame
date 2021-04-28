// license:BSD-3-Clause
// copyright-holders:K.Wilkins,Couriersud,Derrick Renaud,Frank Palazzolo,Jonathan Gevaryahu
/*
    This is an implementation of a Direct-form II digital biquad filter,
    intended for use in audio paths for filtering audio to or from other
    stream devices.

    It has a number of constructor-helpers for automatically generating
    a biquad filter equivalent to the filter response of a few standard
    analog second order filter topographies.

    This biquad filter implementation is based on one written by Frank
    Palazzolo, K. Wilkins, Couriersud, and Derrick Renaud, with some changes:
    * It uses the Q factor directly in the filter definitions, rather than the damping factor (1/Q)
    * It implements every common type of digital biquad filter which I could find documentation for.
    * The filter is Direct-form II instead of Direct-form I, which results in shorter compiled code.

    Possibly useful features which aren't implemented because nothing uses them yet:
    * More Sallen-Key filter variations (band-pass, high-pass)
    * Direct control of the 5 normalized biquad parameters for a custom/raw parameter filter.
*/
#include "emu.h"
#include "flt_biquad.h"

// we need the M_SQRT2 constant
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

// define this to display debug info about the filters being set up
#undef FLT_BIQUAD_DEBUG_SETUP

// device type definition
DEFINE_DEVICE_TYPE(FILTER_BIQUAD, filter_biquad_device, "filter_biquad", "Biquad Filter")

// allow the enum class for the biquad filter type to be saved by the savestate system
ALLOW_SAVE_TYPE(filter_biquad_device::biquad_type);

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  filter_biquad_device - constructor
//-------------------------------------------------

// initialize with some sane defaults for a highpass filter with a cutoff at 16hz, same as flt_rc's 'ac' mode.
filter_biquad_device::filter_biquad_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, FILTER_BIQUAD, tag, owner, clock),
		device_sound_interface(mconfig, *this),
		m_stream(nullptr),
		m_type(biquad_type::HIGHPASS),
		m_last_sample_rate(0),
		m_fc(16.0),
		m_q(M_SQRT2/2.0),
		m_gain(1.0),
		m_input(0.0),
		m_w0(0.0),
		m_w1(0.0),
		m_w2(0.0),
		m_output(0.0),
		m_a1(0.0),
		m_a2(0.0),
		m_b0(1.0),
		m_b1(0.0),
		m_b2(0.0)
{
}

// set up the filter with the specified parameters and return a pointer to the new device
filter_biquad_device& filter_biquad_device::setup(biquad_type type, double fc, double q, double gain)
{
	m_type = type;
	m_fc = fc;
	m_q = q;
	m_gain = gain;
	return *this;
}
filter_biquad_device& filter_biquad_device::setup(filter_biquad_device::biquad_params p)
{
	m_type = p.type;
	m_fc = p.fc;
	m_q = p.q;
	m_gain = p.gain;
	return *this;
}

// modify an existing instance with new filter parameters
void filter_biquad_device::modify(biquad_type type, double fc, double q, double gain)
{
	m_stream->update();
	m_type = type;
	m_fc = fc;
	m_q = q;
	m_gain = gain;
	recalc();
}
void filter_biquad_device::modify(filter_biquad_device::biquad_params p)
{
	m_stream->update();
	m_type = p.type;
	m_fc = p.fc;
	m_q = p.q;
	m_gain = p.gain;
	recalc();
}

//-------------------------------------------------
// Filter setup helpers for various filter models
//-------------------------------------------------

// Sallen-Key filters

/* Setup a biquad filter structure based on a single op-amp Sallen-Key low-pass filter circuit.
 *
 *                   .----------------------------.
 *                   |                            |
 *                  ---  c1                       |
 *                  ---                           |
 *                   |                            |
 *            r1     |   r2                |\     |
 *   In >----ZZZZ----+--ZZZZ---+--------+  | \    |
 *                             |        '--|+ \   |
 *                            ---  c2      |   >--+------> out
 *                            ---       .--|- /   |
 *                             |        |  | /    |
 *                            gnd       |  |/     |
 *                                      |         |
 *                                      |   r4    |
 *                                      +--ZZZZ---'
 *                                      |
 *                                      Z
 *                                      Z r3
 *                                      Z
 *                                      |
 *                                     gnd
 */
filter_biquad_device& filter_biquad_device::opamp_sk_lowpass_setup(double r1, double r2, double r3, double r4, double c1, double c2)
{
	filter_biquad_device::biquad_params p = opamp_sk_lowpass_calc(r1, r2, r3, r4, c1, c2);
	return setup(p);
}

void filter_biquad_device::opamp_sk_lowpass_modify(double r1, double r2, double r3, double r4, double c1, double c2)
{
	filter_biquad_device::biquad_params p = opamp_sk_lowpass_calc(r1, r2, r3, r4, c1, c2);
	modify(p);
}

filter_biquad_device::biquad_params filter_biquad_device::opamp_sk_lowpass_calc(double r1, double r2, double r3, double r4, double c1, double c2)
{
	filter_biquad_device::biquad_params r;
	if ((r1 == 0) || (r2 == 0) || (r3 == 0) || (r4 == 0) || (c1 == 0) || (c2 == 0))
	{
		fatalerror("filter_biquad_device::opamp_sk_lowpass_calc() - no parameters can be 0; parameters were: r1: %f, r2: %f, r3: %f, r4: %f, c1: %f, c2: %f", r1, r2, r3, r4, c1, c2); /* Filter can not be setup.  Undefined results. */
	}
	// NOTE: if R3 doesn't exist (no link to ground), pass a value of RES_M(999.99) or the like, i.e. an 'infinite resistor'
	// NOTE: if R4 is a direct short, set its resistance to RES_R(0.001)
	// NOTE: if R3 doesn't exist AND R4 is a direct short, follow both rules above.
	r.type = biquad_type::LOWPASS;
	r.gain = 1.0 + (r4 / r3); // == (r3 + r4) / r3
	r.fc = 1.0 / (2 * M_PI * sqrt(r1 * r2 * c1 * c2));
	r.q = sqrt(r1 * r2 * c1 * c2) / ((r1 * c2) + (r2 * c2) + ((r2 * c1) * (1.0 - r.gain)));
#ifdef FLT_BIQUAD_DEBUG_SETUP
		logerror("filter_biquad_device::opamp_sk_lowpass_calc(%f, %f, %f, %f, %f, %f) yields: fc = %f, Q = %f, gain = %f\n", r1, r2, r3, r4, c1*1000000, c2*1000000, r.fc, r.q, r.gain);
#endif
	return r;
}

// Multiple-Feedback filters
// (This is sometimes called a 'Rauch' filter circuit.)

/* Setup a biquad filter structure based on a single op-amp Multiple-Feedback low-pass filter circuit.
 * NOTE: vRef is not definable when setting up the filter.
 *  If the analog effects caused by vRef are important to the operation of the specific
 *  filter in question, a netlist implementation may work better under those circumstances.
 * NOTE2: There is a well known 'proper' 1st order version of this circuit where r2 is
 *  a dead short, and c1 omitted. set both c1 and r2 to 0 in this case.
 * NOTE3: a variant of NOTE2 has only the c1 capacitor left off, and r2 present. if so,
 *  set c1 to 0 and r2 to its expected value.
 *
 *                             .--------+---------.
 *                             |        |         |
 *                             Z       --- c2     |
 *                             Z r3    ---        |
 *                             Z        |         |
 *            r1               |   r2   |  |\     |
 *   In >----ZZZZ----+---------+--ZZZZ--+  | \    |
 *                   |                  '--|- \   |
 *                  ---  c1                |   >--+------> out
 *                  ---                 .--|+ /
 *                   |                  |  | /
 *                  gnd        vRef >---'  |/
 *
 */
filter_biquad_device& filter_biquad_device::opamp_mfb_lowpass_setup(double r1, double r2, double r3, double c1, double c2)
{
	filter_biquad_device::biquad_params p = opamp_mfb_lowpass_calc(r1, r2, r3, c1, c2);
	return setup(p);
}

void filter_biquad_device::opamp_mfb_lowpass_modify(double r1, double r2, double r3, double c1, double c2)
{
	filter_biquad_device::biquad_params p = opamp_mfb_lowpass_calc(r1, r2, r3, c1, c2);
	modify(p);
}

filter_biquad_device::biquad_params filter_biquad_device::opamp_mfb_lowpass_calc(double r1, double r2, double r3, double c1, double c2)
{
	filter_biquad_device::biquad_params r;
	if ((r1 == 0) || ((r2 == 0) && (c1 != 0)) || (r3 == 0) || (c2 == 0))
	{
		fatalerror("filter_biquad_device::opamp_mfb_lowpass_calc() - only c1 can be 0 (and if c1 is 0, r2 can also be 0); parameters were: r1: %f, r2: %f, r3: %f, c1: %f, c2: %f", r1, r2, r3, c1, c2); /* Filter can not be setup.  Undefined results. */
	}
	r.gain = -r3 / r1;
	r.q = (M_SQRT2 / 2.0);
	if (c1 == 0) // if both R2 and C1 are 0, it is the 'proper' first order case. There do exist some unusual filters where R2 is not 0, though. In both cases this yields a single-pole filter with limited configurable gain, and a Q of ~0.707. R2 being zero makes the (r1 * r3) numerator term cancel out to 1.0.
	{
		r.fc = (r1 * r3) / (2 * M_PI * ((r1 * r2) + (r1 * r3) + (r2 * r3)) * r3 * c2);
		r.type = biquad_type::LOWPASS1P;
	}
	else // common case, (r2 != 0) && (c1 != 0)
	{
		r.fc = 1.0 / (2 * M_PI * sqrt(r2 * r3 * c1 * c2));
		r.q = sqrt(r2 * r3 * c1 * c2) / ((r3 * c2) + (r2 * c2) + ((r2 * c2) * -r.gain));
		r.type = biquad_type::LOWPASS;
	}
#ifdef FLT_BIQUAD_DEBUG_SETUP
		logerror("filter_biquad_device::opamp_mfb_lowpass_calc(%f, %f, %f, %f, %f) yields:\n\ttype = %d, fc = %f, Q = %f, gain = %f\n", r1, r2, r3, c1*1000000, c2*1000000, static_cast<int>(r.type), r.fc, r.q, r.gain);
#endif
	return r;
}

/* Setup a biquad filter structure based on a single op-amp Multiple-Feedback band-pass filter circuit.
 * NOTE: vRef is not definable when setting up the filter.
 *  If the analog effects caused by vRef are important to the operation of the specific filter
 *  in question, a netlist implementation may work better under those circumstances.
 * NOTE2: If r2 is not used, then set it to 0 ohms, the code will ignore it and assume a fixed gain of 1.
 *
 *                             .--------+---------.
 *                             |        |         |
 *                            --- c1    Z         |
 *                            ---       Z r3      |
 *                             |        Z         |
 *            r1               |  c2    |  |\     |
 *   In >----ZZZZ----+---------+--||----+  | \    |
 *                   Z                  '--|- \   |
 *                   Z r2                  |   >--+------> out
 *                   Z                  .--|+ /
 *                   |                  |  | /
 *                  gnd        vRef >---'  |/
 *
 */
filter_biquad_device& filter_biquad_device::opamp_mfb_bandpass_setup(double r1, double r2, double r3, double c1, double c2)
{
	if ((r1 == 0) || (r3 == 0) || (c1 == 0) || (c2 == 0))
	{
		fatalerror("filter_biquad_device::opamp_mfb_bandpass_setup() - only r2 can be 0; parameters were: r1: %f, r2: %f, r3: %f, c1: %f, c2: %f", r1, r2, r3, c1, c2); /* Filter can not be setup.  Undefined results. */
	}

	double r_in, gain;

	if (r2 == 0)
	{
		gain = 1;
		r_in = r1;
	}
	else
	{
		gain = r2 / (r1 + r2);
		r_in = 1.0 / (1.0/r1 + 1.0/r2);
	}

	double const fc = 1.0 / (2 * M_PI * sqrt(r_in * r3 * c1 * c2)); // technically this is the center frequency of the bandpass
	double const q = sqrt(r3 / r_in * c1 * c2) / (c1 + c2);
	gain *= -r3 / r_in * c2 / (c1 + c2);
#ifdef FLT_BIQUAD_DEBUG_SETUP
	logerror("filter_biquad_device::opamp_mfb_bandpass_setup() yields: fc = %f, Q = %f, gain = %f\n", fc, q, gain);
#endif
	return setup(biquad_type::BANDPASS, fc, q, gain);
}

/* Setup a biquad filter structure based on a single op-amp Multiple-Feedback high-pass filter circuit.
 * NOTE: vRef is not definable when setting up the filter.
 *  If the analog effects caused by vRef are important to the operation of the specific filter
 *  in question, a netlist implementation may work better under those circumstances.
 *
 *                             .--------+---------.
 *                             |        |         |
 *                            --- c3    Z         |
 *                            ---       Z r2      |
 *                             |        Z         |
 *            c1               |   c2   |  |\     |
 *   In >-----||-----+---------+---||---+  | \    |
 *                   Z                  '--|- \   |
 *                   Z r1                  |   >--+------> out
 *                   Z                  .--|+ /
 *                   |                  |  | /
 *                  gnd        vRef >---'  |/
 *
 */
filter_biquad_device& filter_biquad_device::opamp_mfb_highpass_setup(double r1, double r2, double c1, double c2, double c3)
{
	if ((r1 == 0) || (r2 == 0) || (c1 == 0) || (c2 == 0) || (c3 == 0))
	{
		fatalerror("filter_biquad_device::opamp_mfb_highpass_setup() - no parameters can be 0; parameters were: r1: %f, r2: %f, c1: %f, c2: %f, c3: %f", r1, r2, c1, c2, c3); /* Filter can not be setup.  Undefined results. */
	}

	double const gain = -c1 / c3;
	double const fc = 1.0 / (2 * M_PI * sqrt(c2 * c3 * r1 * r2));
	double const q = sqrt(c2 * c3 * r1 * r2) / ((c2 * r1) + (c3 * r1) + ((c3 * r1) * -gain));
#ifdef FLT_BIQUAD_DEBUG_SETUP
	logerror("filter_biquad_device::opamp_mfb_highpass_setup() yields: fc = %f, Q = %f, gain = %f\n", fc, q, gain);
#endif
	return setup(biquad_type::HIGHPASS, fc, q, gain);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void filter_biquad_device::device_start()
{
	m_stream = stream_alloc(1, 1, SAMPLE_RATE_OUTPUT_ADAPTIVE);
	m_last_sample_rate = 0;
	recalc();

	save_item(NAME(m_type));
	save_item(NAME(m_last_sample_rate));
	save_item(NAME(m_fc));
	save_item(NAME(m_q));
	save_item(NAME(m_gain));
	save_item(NAME(m_input));
	save_item(NAME(m_w0));
	save_item(NAME(m_w1));
	save_item(NAME(m_w2));
	save_item(NAME(m_output));
	save_item(NAME(m_a1));
	save_item(NAME(m_a2));
	save_item(NAME(m_b0));
	save_item(NAME(m_b1));
	save_item(NAME(m_b2));
}


//-------------------------------------------------
//  sound_stream_update - handle a stream update
//-------------------------------------------------

void filter_biquad_device::sound_stream_update(sound_stream &stream, std::vector<read_stream_view> const &inputs, std::vector<write_stream_view> &outputs)
{
	auto &src = inputs[0];
	auto &dst = outputs[0];

	if (m_last_sample_rate != m_stream->sample_rate())
	{
		recalc();
		m_last_sample_rate = m_stream->sample_rate();
	}

	for (int sampindex = 0; sampindex < dst.samples(); sampindex++)
	{
		m_input = src.get(sampindex);
		step();
		dst.put(sampindex, m_output);
	}
}


/* Calculate the filter context based on the passed filter type info.
 * m_type - 1 of the 9 defined filter types
 * m_fc   - center frequency
 * m_q    - 'Q' (quality) factor of filter (1/damp)
 * m_gain - overall filter gain. Set to 1.0 if not needed. The exact meaning of gain changes depending on the filter type.
 */
void filter_biquad_device::recalc()
{
	double const MGain = fabs(m_gain); // absolute multiplicative gain
	double const DBGain = log10(MGain) * 20.0; // gain in dB
	double const AMGain = pow(10, fabs(DBGain) / 20.0); // multiplicative gain of absolute DB
	double const K = tan(M_PI * m_fc / m_stream->sample_rate());
	double const Ksquared = K * K;
	double const KoverQ = K / m_q;
	double normal = 1.0 / (1.0 + KoverQ + Ksquared);

	m_a1 = 2.0 * (Ksquared - 1.0) * normal;
	m_a2 = (1.0 - KoverQ + Ksquared) * normal;

	switch (m_type)
	{
		case biquad_type::LOWPASS1P:
			m_a1 = exp(-2.0 * M_PI * (m_fc / m_stream->sample_rate()));
			m_b0 = 1.0 - m_a1;
			m_a1 = -m_a1;
			m_b1 = m_b2 = m_a2 = 0.0;
			break;
		case biquad_type::HIGHPASS1P:
			m_a1 = -exp(-2.0 * M_PI * (0.5 - m_fc / m_stream->sample_rate()));
			m_b0 = 1.0 + m_a1;
			m_a1 = -m_a1;
			m_b1 = m_b2 = m_a2 = 0.0;
			break;
		case biquad_type::LOWPASS:
			m_b0 = Ksquared * normal;
			m_b1 = 2.0 * m_b0;
			m_b2 = 1.0 * m_b0;
			m_a1 = 2.0 * (Ksquared - 1.0) * normal;
			m_a2 = (1.0 - KoverQ + Ksquared) * normal;
			break;
		case biquad_type::HIGHPASS:
			m_b0 = 1.0 * normal;
			m_b1 = -2.0 * m_b0;
			m_b2 = 1.0 * m_b0;
			m_a1 = 2.0 * (Ksquared - 1.0) * normal;
			m_a2 = (1.0 - KoverQ + Ksquared) * normal;
			break;
		case biquad_type::BANDPASS:
			m_b0 = KoverQ * normal;
			m_b1 = 0.0;
			m_b2 = -1.0 * m_b0;
			m_a1 = 2.0 * (Ksquared - 1.0) * normal;
			m_a2 = (1.0 - KoverQ + Ksquared) * normal;
			break;
		case biquad_type::NOTCH:
			m_b0 = (1.0 + Ksquared) * normal;
			m_b1 = 2.0 * (Ksquared - 1.0) * normal;
			m_b2 = 1.0 * m_b0;
			m_a1 = 1.0 * m_b1;
			m_a2 = (1.0 - KoverQ + Ksquared) * normal;
			break;
		case biquad_type::PEAK:
			if (DBGain >= 0.0)
			{
				m_b0 = (1.0 + (AMGain * KoverQ) + Ksquared) * normal;
				m_b1 = 2.0 * (Ksquared - 1.0) * normal;
				m_b2 = (1.0 - (AMGain * KoverQ) + Ksquared) * normal;
				m_a1 = 1.0 * m_b1;
				m_a2 = (1.0 - KoverQ + Ksquared) * normal;
			}
			else
			{
				normal = 1.0 / (1.0 + (AMGain * KoverQ) + Ksquared);
				m_b0 = (1.0 + KoverQ + Ksquared) * normal;
				m_b1 = 2.0 * (Ksquared - 1.0) * normal;
				m_b2 = (1.0 - KoverQ + Ksquared) * normal;
				m_a1 = 1.0 * m_b1;
				m_a2 = (1.0 - (AMGain * KoverQ) + Ksquared) * normal;
			}
			break;
		case biquad_type::LOWSHELF:
			if (DBGain >= 0.0)
			{
				normal = 1.0 / (1.0 + M_SQRT2 * K + Ksquared);
				m_b0 = (1.0 + sqrt(2.0 * AMGain) * K + AMGain * Ksquared) * normal;
				m_b1 = 2.0 * (AMGain * Ksquared - 1.0) * normal;
				m_b2 = (1.0 - sqrt(2.0 * AMGain) * K + AMGain * Ksquared) * normal;
				m_a1 = 2.0 * (Ksquared - 1.0) * normal;
				m_a2 = (1.0 - M_SQRT2 * K + Ksquared) * normal;
			}
			else
			{
				normal = 1.0 / (1.0 + sqrt(2.0 * AMGain) * K + AMGain * Ksquared);
				m_b0 = (1.0 + M_SQRT2 * K + Ksquared) * normal;
				m_b1 = 2.0 * (Ksquared - 1.0) * normal;
				m_b2 = (1.0 - M_SQRT2 * K + Ksquared) * normal;
				m_a1 = 2.0 * (AMGain * Ksquared - 1.0) * normal;
				m_a2 = (1.0 - sqrt(2.0 * AMGain) * K + AMGain * Ksquared) * normal;
			}
			break;
		case biquad_type::HIGHSHELF:
			if (DBGain >= 0.0)
			{
				normal = 1.0 / (1.0 + M_SQRT2 * K + Ksquared);
				m_b0 = (AMGain + sqrt(2.0 * AMGain) * K + Ksquared) * normal;
				m_b1 = 2.0 * (Ksquared - AMGain) * normal;
				m_b2 = (AMGain - sqrt(2.0 * AMGain) * K + Ksquared) * normal;
				m_a1 = 2.0 * (Ksquared - 1.0) * normal;
				m_a2 = (1.0 - M_SQRT2 * K + Ksquared) * normal;
			}
			else
			{
				normal = 1.0 / (AMGain + sqrt(2.0 * AMGain) * K + Ksquared);
				m_b0 = (1.0 + M_SQRT2 * K + Ksquared) * normal;
				m_b1 = 2.0 * (Ksquared - 1.0) * normal;
				m_b2 = (1.0 - M_SQRT2 * K + Ksquared) * normal;
				m_a1 = 2.0 * (Ksquared - AMGain) * normal;
				m_a2 = (AMGain - sqrt(2.0 * AMGain) * K + Ksquared) * normal;
			}
			break;
		default:
			fatalerror("filter_biquad_device::recalc() - Invalid filter type!");
			break;
	}
#ifdef FLT_BIQUAD_DEBUG
	logerror("Calculated Parameters:\n");
	logerror( "Gain (dB): %f, (raw): %f\n", DBGain, MGain);
	logerror( "k: %f\n", K);
	logerror( "normal: %f\n", normal);
	logerror("b0: %f\n", m_b0);
	logerror("b1: %f\n", m_b1);
	logerror("b2: %f\n", m_b2);
	logerror("a1: %f\n", m_a1);
	logerror("a2: %f\n", m_a2);
#endif
	// peak and shelf filters do not use gain for the entire signal, only for the peak/shelf portions
	// side note: the first order lowpass and highpass filter analogs technically don't have gain either,
	// but this can be 'faked' by adjusting the bx factors, so we support that anyway, even if it isn't realistic.
	if ( (m_type != biquad_type::PEAK)
		&& (m_type != biquad_type::LOWSHELF)
		&& (m_type != biquad_type::HIGHSHELF) )
	{
		m_b0 *= m_gain;
		m_b1 *= m_gain;
		m_b2 *= m_gain;
#ifdef FLT_BIQUAD_DEBUG
		logerror("b0g: %f\n", m_b0);
		logerror("b1g: %f\n", m_b1);
		logerror("b2g: %f\n", m_b2);
#endif
	}
}

/* Step the filter */
void filter_biquad_device::step()
{
	m_w2 = m_w1;
	m_w1 = m_w0;
	m_w0 = (-m_a1 * m_w1) + (-m_a2 * m_w2) + m_input;
	m_output = (m_b0 * m_w0) + (m_b1 * m_w1) + (m_b2 * m_w2);
}
