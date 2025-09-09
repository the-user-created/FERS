/**
 * @file radar_signal.h
 * @brief Classes for handling radar waveforms and signals.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-06-08
 */

#pragma once

#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#include "config.h"

namespace interp
{
	struct InterpPoint;
}

namespace fers_signal
{
	/**
	 * @class Signal
	 * @brief Class for handling radar waveform signal data.
	 */
	class Signal
	{
	public:
		/**
		 * @brief Clears the internal signal data.
		 */
		void clear() noexcept;

		/**
		 * @brief Loads complex radar waveform data.
		 *
		 * @param inData The input span of complex signal data.
		 * @param samples The number of samples in the input data.
		 * @param sampleRate The sample rate of the input data.
		 */
		void load(std::span<const ComplexType> inData, unsigned samples, RealType sampleRate);

		/**
		 * @brief Gets the sample rate of the signal.
		 *
		 * @return The sample rate of the signal.
		 */
		[[nodiscard]] RealType getRate() const noexcept { return _rate; }

		/**
		 * @brief Renders the signal data based on interpolation points.
		 *
		 * @param points A vector of interpolation points used to render the signal.
		 * @param size Reference to store the size of the rendered data.
		 * @param fracWinDelay Fractional window delay to apply during rendering.
		 * @return A vector of rendered complex signal data.
		 */
		std::vector<ComplexType> render(const std::vector<interp::InterpPoint>& points, unsigned& size,
		                                double fracWinDelay) const;

	private:
		std::vector<ComplexType> _data; ///< The complex signal data.
		unsigned _size{0}; ///< The size of the signal data.
		RealType _rate{0}; ///< The sample rate of the signal.

		/**
		 * @brief Calculates weights and delays for rendering.
		 *
		 * @param iter Iterator pointing to the current interpolation point.
		 * @param next Iterator pointing to the next interpolation point.
		 * @param sampleTime Current sample time.
		 * @param idelay Integer delay value.
		 * @param fracWinDelay Fractional window delay to apply.
		 * @return A tuple containing amplitude, phase, fractional delay, and sample unwrap index.
		 */
		[[nodiscard]] constexpr std::tuple<double, double, double, int> calculateWeightsAndDelays(
			std::vector<interp::InterpPoint>::const_iterator iter,
			std::vector<interp::InterpPoint>::const_iterator next, double sampleTime, double idelay,
			double fracWinDelay) const noexcept;

		/**
		 * @brief Performs convolution with a filter.
		 *
		 * @param i Index of the current sample.
		 * @param filt Pointer to the filter coefficients.
		 * @param filtLength Length of the filter.
		 * @param amplitude Amplitude scaling factor.
		 * @param iSampleUnwrap Unwrapped sample index for the convolution.
		 * @return The result of the convolution for the given sample.
		 */
		ComplexType performConvolution(int i, const double* filt, int filtLength, double amplitude,
		                               int iSampleUnwrap) const noexcept;
	};

	/**
	 * @class RadarSignal
	 * @brief Class representing a radar signal with associated properties.
	 */
	class RadarSignal
	{
	public:
		/**
		 * @brief Constructs a RadarSignal object.
		 *
		 * @param name The name of the radar signal.
		 * @param power The power of the radar signal.
		 * @param carrierfreq The carrier frequency of the radar signal.
		 * @param length The length of the radar signal.
		 * @param signal A unique pointer to the `Signal` object containing the waveform data.
		 * @throws std::runtime_error if the signal is null.
		 */
		RadarSignal(std::string name, RealType power, RealType carrierfreq, RealType length,
		            std::unique_ptr<Signal> signal);

		~RadarSignal() = default;
		RadarSignal(const RadarSignal&) noexcept = delete;
		RadarSignal& operator=(const RadarSignal&) noexcept = delete;
		RadarSignal(RadarSignal&&) noexcept = delete;
		RadarSignal& operator=(RadarSignal&&) noexcept = delete;

		/**
		 * @brief Gets the power of the radar signal.
		 *
		 * @return The power of the radar signal.
		 */
		[[nodiscard]] RealType getPower() const noexcept { return _power; }

		/**
		 * @brief Gets the carrier frequency of the radar signal.
		 *
		 * @return The carrier frequency of the radar signal.
		 */
		[[nodiscard]] RealType getCarrier() const noexcept { return _carrierfreq; }

		/**
		 * @brief Gets the name of the radar signal.
		 *
		 * @return The name of the radar signal.
		 */
		[[nodiscard]] const std::string& getName() const noexcept { return _name; }

		/**
		 * @brief Gets the sample rate of the radar signal.
		 *
		 * @return The sample rate of the radar signal.
		 */
		[[nodiscard]] RealType getRate() const noexcept { return _signal->getRate(); }

		/**
		 * @brief Gets the length of the radar signal.
		 *
		 * @return The length of the radar signal.
		 */
		[[nodiscard]] RealType getLength() const noexcept { return _length; }

		/**
		 * @brief Renders the radar signal.
		 *
		 * @param points A vector of interpolation points.
		 * @param size Reference to store the size of the rendered data.
		 * @param fracWinDelay Fractional window delay to apply during rendering.
		 * @return A vector of rendered complex radar signal data.
		 */
		std::vector<ComplexType> render(const std::vector<interp::InterpPoint>& points, unsigned& size,
		                                RealType fracWinDelay) const;

	private:
		std::string _name; ///< The name of the radar signal.
		RealType _power; ///< The power of the radar signal.
		RealType _carrierfreq; ///< The carrier frequency of the radar signal.
		RealType _length; ///< The length of the radar signal.
		std::unique_ptr<Signal> _signal; ///< The `Signal` object containing the radar signal data.
	};
}
