/**
 * @file radar_signal.h
 * @brief Classes for handling radar waveforms and signals.
 *
 * This header defines the classes that represent radar signals and the underlying
 * signal data for radar waveforms. The `Signal` class handles loading, rendering,
 * and processing of the radar waveform data. The `RadarSignal` class represents a
 * radar signal with its parameters, such as power, frequency, and length, and
 * provides an interface for rendering the signal.
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

namespace signal
{
	/**
	 * @class Signal
	 * @brief Class for handling radar waveform signal data.
	 *
	 * This class manages the signal data of a radar waveform. It provides
	 * functionality to load signal data, clear the signal, and render the signal
	 * based on a set of interpolation points. It also includes private methods for
	 * convolution and calculating delays.
	 */
	class Signal
	{
	public:
		/**
		 * @brief Default constructor for the Signal class.
		 */
		Signal() = default;

		/**
		 * @brief Default destructor for the Signal class.
		 */
		~Signal() = default;

		/**
		 * @brief Clears the internal signal data.
		 *
		 * This method resets the signal's internal data, size, and sample rate.
		 */
		void clear() noexcept;

		/**
		 * @brief Loads complex radar waveform data.
		 *
		 * Loads the signal data from a span of complex values and applies the
		 * appropriate oversampling ratio. The sample rate is adjusted according to
		 * the oversampling factor.
		 *
		 * @param inData The input span of complex signal data.
		 * @param samples The number of samples in the input data.
		 * @param sampleRate The sample rate of the input data.
		 */
		void load(std::span<const ComplexType> inData, unsigned samples, RealType sampleRate);

		// Note: This function is not used in the codebase
		//void load(std::span<const RealType> inData, unsigned samples, RealType sampleRate);

		/**
		 * @brief Gets the sample rate of the signal.
		 *
		 * @return The sample rate of the signal.
		 */
		[[nodiscard]] RealType getRate() const noexcept { return _rate; }

		// Note: This function is not used in the codebase
		// [[nodiscard]] unsigned getSize() const noexcept { return _size; }

		/**
		 * @brief Renders the signal data based on interpolation points.
		 *
		 * This method uses a set of interpolation points to render the radar signal
		 * data, applying a fractional window delay.
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
		 * This method computes the amplitude, phase, fractional delay, and unwrapped
		 * sample index based on the current and next interpolation points.
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
		 * Convolves the signal data at the specified index with the given filter
		 * and applies an amplitude adjustment.
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
	 *
	 * This class encapsulates a radar signal with attributes such as name, power,
	 * carrier frequency, length, and an associated `Signal` object. It provides
	 * methods to access these properties and to render the signal based on
	 * interpolation points.
	 */
	class RadarSignal
	{
	public:
		/**
		 * @brief Constructs a RadarSignal object.
		 *
		 * Initializes the radar signal with its name, power, carrier frequency,
		 * length, and an associated `Signal` object.
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

		/**
		 * @brief Destructor for the RadarSignal class.
		 */
		~RadarSignal() = default;

		/**
		 * @brief Deleted copy constructor for the RadarSignal class to prevent copying.
		 */
		RadarSignal(const RadarSignal&) noexcept = delete;

		/**
		 * @brief Deleted copy assignment operator for the RadarSignal class to prevent copying.
		 */
		RadarSignal& operator=(const RadarSignal&) noexcept = delete;

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
		 * This method renders the radar signal based on a set of interpolation
		 * points, applying a scaling factor based on the signal's power and a
		 * fractional window delay.
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
