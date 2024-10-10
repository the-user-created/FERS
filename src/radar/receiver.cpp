/**
 * @file receiver.cpp
 * @brief Implementation of the Receiver class.
 *
 * The `Receiver` class extends the `Radar` class to provide additional features
 * related to signal reception, such as noise temperature management, window properties, and response collection.
 * It supports multiple configuration flags and the ability to work in dual receiver mode.
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

		// Export based on user preferences
		if (params::exportXml()) { exportReceiverXml(_responses, getName() + "_results"); }
		if (params::exportCsv()) { exportReceiverCsv(_responses, getName() + "_results"); }
		if (params::exportBinary()) { exportReceiverBinary(_responses, this, getName() + "_results", pool); }
	}

	void Receiver::setWindowProperties(const RealType length, const RealType prf, const RealType skip) noexcept
	{
		const auto rate = params::rate() * params::oversampleRatio();
		_window_length = length;
		_window_prf = 1 / (std::floor(rate / prf) / rate); // Update prf to precise value
		_window_skip = std::floor(rate * skip) / rate; // Update skip with better precision
	}

	int Receiver::getWindowCount() const noexcept
	{
		const RealType time = params::endTime() - params::startTime();
		const RealType pulses = time * _window_prf;
		return static_cast<int>(std::ceil(pulses));
	}

	RealType Receiver::getWindowStart(const int window) const
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
