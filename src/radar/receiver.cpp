/**
 * @file receiver.cpp
 * @brief Implementation of the Receiver class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-10-07
 */

#include "receiver.h"

#include <algorithm>
#include <highfive/highfive.hpp>

#include "core/parameters.h"
#include "serial/hdf5_handler.h"
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

	void Receiver::exportCwData(pool::ThreadPool& /* pool */)
	{
		const auto hdf5_filename = std::format("{}_results.h5", getName());
		try
		{
			HighFive::File file(hdf5_filename, HighFive::File::Truncate);

			std::lock_guard lock(m_cpiMutex);
			for (size_t i = 0; i < m_cpiDataBlocks.size(); ++i)
			{
				const auto& iq_block = m_cpiDataBlocks[i];
				const RealType start_time = m_cpiStartTimes[i];
				const RealType carrier_freq = m_cpiCarrierFreqs[i];

				// Phase 3 will add noise/quantization here. For now, fullscale is 1.0.
				serial::addChunkToFile(file, iq_block, start_time, 1.0, i, m_samplingRate, carrier_freq);
			}
		}
		catch (const HighFive::Exception& err)
		{
			LOG(logging::Level::FATAL, "Error creating HDF5 file {}: {}", hdf5_filename, err.what());
			throw std::runtime_error("Error creating HDF5 file " + hdf5_filename);
		}
	}

	void Receiver::render(pool::ThreadPool& pool)
	{
		if (m_isCwReceiver)
		{
			if (params::exportBinary()) { exportCwData(pool); }
			return;
		}

		if (_responses.empty())
		{
			LOG(logging::Level::INFO, "No responses to render for receiver {}", getName());
			return;
		}

		std::ranges::sort(_responses, serial::compareTimes);

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

	void Receiver::setCwProcessing(const RealType cpiDuration, const RealType samplingRate,
	                               const RealType cpiOverlap) noexcept
	{
		m_isCwReceiver = true;
		m_cpiDuration = cpiDuration;
		m_samplingRate = samplingRate;
		m_cpiOverlap = cpiOverlap;
	}

	void Receiver::addCpiDataBlock(std::vector<ComplexType> block, const RealType startTime, const RealType carrierFreq)
	{
		std::lock_guard lock(m_cpiMutex);
		m_cpiDataBlocks.push_back(std::move(block));
		m_cpiStartTimes.push_back(startTime);
		m_cpiCarrierFreqs.push_back(carrierFreq);
	}
}
