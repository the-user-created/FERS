// falpha_branch.cpp
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "falpha_branch.h"

#include <array>
#include <cmath>

#include "noise_utils.h"
#include "core/logging.h"

// =====================================================================================================================
//
// FALPHA BRANCH CLASS
//
// =====================================================================================================================

namespace rs
{
	FAlphaBranch::FAlphaBranch(const RS_FLOAT ffrac, const unsigned fint, std::unique_ptr<FAlphaBranch> pre,
	                           const bool last) : _pre(std::move(pre)), _last(last), _ffrac(ffrac), _fint(fint)
	{
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Making branch ffrac=%f fint=%d\n", ffrac, fint);
		_upsample_scale = std::pow(10, ffrac + fint + 0.5);
		init();
		_buffer = std::make_unique<RS_FLOAT[]>(10);
		if (!last) { refill(); }
	}

	void FAlphaBranch::init()
	{
		_upsampler = std::make_unique<DecadeUpsampler>();

		if (_pre)
		{
			constexpr std::array hp_num = {
				3.817871081981451e-01, -4.093384095523618e+00, 2.005300512623078e+01, -5.924672881811163e+01,
				1.172948159891025e+02, -1.633810410083022e+02, 1.633810410083034e+02, -1.172948159891052e+02,
				5.924672881811390e+01, -2.005300512623186e+01, 4.093384095523903e+00, -3.817871081981776e-01
			};
			constexpr std::array hp_den = {
				1.000000000000000e+00, -8.829695665523831e+00, 3.583068809011030e+01, -8.811479652970442e+01,
				1.457874067329429e+02, -1.702715637111961e+02, 1.431504350055831e+02, -8.656925883534657e+01,
				3.687395592491803e+01, -1.052413841411803e+01, 1.808292123637038e+00, -1.412932578340511e-01
			};
			_highpass = std::make_unique<IirFilter>(hp_den.data(), hp_num.data(), hp_num.size());
		}

		if (_ffrac == 0.5f)
		{
			constexpr std::array sf_num = {
				5.210373977738306e-03, -7.694671394585578e-03, 1.635979377907092e-03, 9.852449140857658e-05,
				-2.080553126780113e-03, 4.088764157029523e-03, -1.549082440084623e-03, 9.054734252370680e-04,
				-3.467369912368729e-04, 4.516383087838856e-04, -1.063356106118517e-03, 1.330008998057684e-04,
				6.556909567323943e-04, -4.839476350293955e-04, 6.664936170526832e-05, 1.528520559763056e-05
			};
			constexpr std::array sf_den = {
				1.000000000000000e+00, -2.065565041154101e+00, 1.130909190864681e+00, -1.671244644503288e-01,
				-3.331474931013877e-01, 9.952625337612708e-01, -7.123036343635182e-01, 3.297062696290504e-01,
				-1.925691520710595e-01, 1.301247006176314e-01, -2.702016290409912e-01, 1.455380885858886e-01,
				1.091921868353888e-01, -1.524953111510459e-01, 5.667716332023935e-02, -2.890314873767405e-03
			};
			_shape_gain = 5.210373977738306e-03;
			_shape_filter = std::make_unique<IirFilter>(sf_den.data(), sf_num.data(), sf_num.size());
		}
		else if (_ffrac != 0.0f)
		{
			logging::printf(logging::RS_CRITICAL, "[CRITICAL] Value of ffrac is %f\n", _ffrac);
			throw std::runtime_error("Fractional integrator values other than 0.5 are not supported");
		}

		if (_fint > 0)
		{
			_integ_gain = 1.0f;

			if (_fint == 1)
			{
				constexpr std::array<RS_FLOAT, 2> i_den = {1.0f, -1.0f};
				constexpr std::array<RS_FLOAT, 2> i_num = {1.0f, 0.0f};
				_integ_filter = std::make_unique<IirFilter>(i_den.data(), i_num.data(), i_num.size());
			}
			else if (_fint == 2)
			{
				constexpr std::array<RS_FLOAT, 3> i_den = {1.0f, -2.0f, 1.0f};
				constexpr std::array<RS_FLOAT, 3> i_num = {1.0f, 0.0f, 0.0f};
				_integ_filter = std::make_unique<IirFilter>(i_den.data(), i_num.data(), i_num.size());
			}
			else { throw std::runtime_error("Only alpha values between 2 and -2 are supported for noise generation"); }
		}

		_offset_sample = 0.0f;
		_got_offset = false;
		_buffer = std::make_unique<RS_FLOAT[]>(10);

		if (!_last) { refill(); }

		_pre_scale = 1.0f;
	}

	RS_FLOAT FAlphaBranch::getSample() // NOLINT(misc-no-recursion)
	{
		if (!_last)
		{
			const RS_FLOAT ret = _buffer[_buffer_samples++];
			if (_buffer_samples == 10) { refill(); }
			return ret;
		}
		return calcSample() + _offset_sample * _upsample_scale;
	}

	void FAlphaBranch::clean()
	{
		_highpass.reset();
		_integ_filter.reset();
		_shape_filter.reset();
		_upsampler.reset();
	}

	RS_FLOAT FAlphaBranch::calcSample() // NOLINT(misc-no-recursion)
	{
		RS_FLOAT sample = noise_utils::wgnSample(1);

		if (_shape_filter) { sample = _shape_filter->filter(sample) / _shape_gain; }

		if (_integ_filter) { sample = _integ_filter->filter(sample) / _integ_gain; }

		if (_pre)
		{
			sample = _highpass->filter(sample);
			if (_got_offset) { sample += _pre->getSample() * _pre_scale - _offset_sample; }
			else
			{
				_got_offset = true;
				_offset_sample = _pre->getSample() * _pre_scale;
			}
		}

		return sample;
	}

	void FAlphaBranch::refill() // NOLINT(misc-no-recursion)
	{
		const RS_FLOAT sample = calcSample();
		_upsampler->upsample(sample, _buffer.get());

		for (int i = 0; i < 10; ++i)
		{
			_buffer[i] *= _upsample_scale;
			_buffer[i] += _offset_sample;
		}

		_buffer_samples = 0;
	}

	void FAlphaBranch::flush(const RS_FLOAT scale)
	{
		clean();
		init();
		_pre_scale = scale;
	}
}