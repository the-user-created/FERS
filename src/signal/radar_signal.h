// radar_signal.h
// Classes for different types of radar waveforms
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 8 June 2006

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
	class Signal
	{
	public:
		Signal() = default;

		~Signal() = default;

		void clear() noexcept;

		void load(std::span<const ComplexType> inData, unsigned samples, RealType sampleRate);

		// Note: This function is not used in the codebase
		//void load(std::span<const RealType> inData, unsigned samples, RealType sampleRate);

		[[nodiscard]] RealType getRate() const noexcept { return _rate; }
		// Note: This function is not used in the codebase
		// [[nodiscard]] unsigned getSize() const noexcept { return _size; }

		std::vector<ComplexType> render(const std::vector<interp::InterpPoint>& points, unsigned& size,
		                                double fracWinDelay) const;

	private:
		std::vector<ComplexType> _data;
		unsigned _size{0};
		RealType _rate{0};

		[[nodiscard]] constexpr std::tuple<double, double, double, int> calculateWeightsAndDelays(
			std::vector<interp::InterpPoint>::const_iterator iter,
			std::vector<interp::InterpPoint>::const_iterator next, double sampleTime, double idelay,
			double fracWinDelay) const noexcept;

		ComplexType performConvolution(int i, const double* filt, int filtLength, double amplitude,
		                               int iSampleUnwrap) const noexcept;
	};

	class RadarSignal
	{
	public:
		RadarSignal(std::string name, RealType power, RealType carrierfreq, RealType length,
		            std::unique_ptr<Signal> signal);

		~RadarSignal() = default;

		// Delete copy constructor and copy assignment operator to prevent copying
		RadarSignal(const RadarSignal&) noexcept = delete;

		RadarSignal& operator=(const RadarSignal&) noexcept = delete;

		[[nodiscard]] RealType getPower() const noexcept { return _power; }
		[[nodiscard]] RealType getCarrier() const noexcept { return _carrierfreq; }
		[[nodiscard]] const std::string& getName() const noexcept { return _name; }
		[[nodiscard]] RealType getRate() const noexcept { return _signal->getRate(); }
		[[nodiscard]] RealType getLength() const noexcept { return _length; }

		std::vector<ComplexType> render(const std::vector<interp::InterpPoint>& points, unsigned& size,
		                                RealType fracWinDelay) const;

	private:
		std::string _name;
		RealType _power;
		RealType _carrierfreq;
		RealType _length;
		std::unique_ptr<Signal> _signal;
	};
}