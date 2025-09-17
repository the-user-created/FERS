// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file receiver.cpp
* @brief Implementation of the Receiver class.
*/

#include "receiver.h"

#include <algorithm>

#include "core/parameters.h"
#include "serial/receiver_export.h"
#include "serial/response.h"

namespace radar
{
	Receiver::Receiver(Platform* platform, std::string name, const unsigned seed) noexcept :
		Radar(platform, std::move(name)),
		_rng(seed) {}

	void Receiver::addResponse(std::unique_ptr<serial::Response> response) noexcept
	{
		std::lock_guard lock(_responses_mutex);
		_responses.push_back(std::move(response));
	}

	RealType Receiver::getNoiseTemperature(const math::SVec3& angle) const noexcept
	{
		return _noise_temperature + Radar::getNoiseTemperature(angle);
	}

	int Receiver::getResponseCount() const noexcept
	{
		return static_cast<int>(_responses.size());
	}

	void Receiver::setNoiseTemperature(const RealType temp)
	{
		if (temp < -EPSILON)
		{
			LOG(logging::Level::FATAL, "Noise temperature for receiver {} is negative", getName());
			throw std::runtime_error("Noise temperature must be positive");
		}
		_noise_temperature = temp;
	}

	void Receiver::render(pool::ThreadPool& pool)
	{
		if (params::isCwSimulation())
		{
			// CW simulation does not support XML or CSV response export
			if (params::exportBinary() && !_cw_iq_data.empty())
			{
				serial::exportReceiverCwBinary(this, getName() + "_results");
			}
		}
		else
		{
			// Prevent exporting empty files when there are no responses
			if (_responses.empty())
			{
				LOG(logging::Level::INFO, "Receiver '{}' has no responses to render. Skipping export.", getName());
				return;
			}
			std::ranges::sort(_responses, serial::compareTimes);

			if (params::exportXml()) { exportReceiverXml(_responses, getName() + "_results"); }
			if (params::exportCsv()) { exportReceiverCsv(_responses, getName() + "_results"); }
			if (params::exportBinary()) { exportReceiverBinary(_responses, this, getName() + "_results", pool); }
		}
	}

	void Receiver::setWindowProperties(const RealType length, const RealType prf, const RealType skip) noexcept
	{
		const auto rate = params::rate() * params::oversampleRatio();
		_window_length = length;
		_window_prf = 1 / (std::floor(rate / prf) / rate);
		_window_skip = std::floor(rate * skip) / rate;
	}

	unsigned Receiver::getWindowCount() const noexcept
	{
		const RealType time = params::endTime() - params::startTime();
		const RealType pulses = time * _window_prf;
		return static_cast<unsigned>(std::ceil(pulses));
	}

	RealType Receiver::getWindowStart(const unsigned window) const
	{
		const RealType stime = static_cast<RealType>(window) / _window_prf + _window_skip;
		if (!_timing)
		{
			LOG(logging::Level::FATAL, "Receiver must be associated with timing source");
			throw std::logic_error("Receiver must be associated with timing source");
		}
		return stime;
	}

	void Receiver::prepareCwData(const size_t numSamples)
	{
		std::lock_guard lock(_cw_mutex);
		_cw_iq_data.resize(numSamples);
	}

	void Receiver::setCwSample(const size_t index, const ComplexType sample)
	{
		if (index < _cw_iq_data.size())
		{
			_cw_iq_data[index] = sample;
		}
	}
}
