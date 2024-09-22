// radar_signal.h
// Classes for different types of radar waveforms
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 8 June 2006

#ifndef RADAR_SIGNAL_H
#define RADAR_SIGNAL_H

#include <memory>
#include <span>
#include <vector>

#include "config.h"
#include "jones_vector.h"

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

		void clear();

		void load(std::span<const ComplexType> inData, unsigned samples, RealType sampleRate);

		// Note: This function is not used in the codebase
		void load(std::span<const RealType> inData, unsigned samples, RealType sampleRate);

		[[nodiscard]] RealType getRate() const noexcept { return _rate; }
		[[nodiscard]] unsigned getSize() const noexcept { return _size; }

		// Note: This function is not used in the codebase
		[[nodiscard]] std::vector<RealType> copyData() const;

		std::vector<ComplexType> render(const std::vector<interp::InterpPoint>& points, unsigned& size,
		                                double fracWinDelay) const;

	private:
		std::vector<ComplexType> _data;
		unsigned _size{0};
		RealType _rate{0};

		[[nodiscard]] constexpr std::tuple<double, double, double, int> calculateWeightsAndDelays(
			std::vector<interp::InterpPoint>::const_iterator iter,
			std::vector<interp::InterpPoint>::const_iterator next, double sampleTime, double idelay,
			double fracWinDelay) const;

		ComplexType performConvolution(int i, const double* filt, int filtLength, double amplitude,
		                               int iSampleUnwrap) const;
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

		// Note: This function is not used in the codebase
		[[nodiscard]] JonesVector getPolarization() const noexcept { return _polar; }

		// Note: This function is not used in the codebase
		void setPolarization(const JonesVector& in) noexcept { _polar = in; }

	private:
		std::string _name;
		RealType _power;
		RealType _carrierfreq;
		RealType _length;
		std::unique_ptr<Signal> _signal;
		JonesVector _polar;
	};
}

#endif
