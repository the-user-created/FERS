/**
 * @file receiver.cpp
 * @brief Implementation of the Receiver class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-10-07
 */

#include "receiver.h"

#include <algorithm>

#include "core/parameters.h"
#include "serial/receiver_export.h"
#include "serial/response.h"

namespace radar
{
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
		std::ranges::sort(_responses, serial::compareTimes);
		// TODO: Should check here if there are any responses to render

		if (params::exportXml()) { exportReceiverXml(_responses, getName() + "_results"); }
		if (params::exportCsv()) { exportReceiverCsv(_responses, getName() + "_results"); }
		if (params::exportBinary()) { exportReceiverBinary(_responses, this, getName() + "_results", pool); }
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
}
