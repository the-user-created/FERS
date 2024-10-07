//
// Created by David Young on 10/7/24.
//

#pragma once

#include "radar_obj.h"

namespace signal
{
	class RadarSignal;
}

namespace radar
{
	struct TransmitterPulse
	{
		signal::RadarSignal* wave;
		RealType time;
	};

	class Transmitter final : public Radar
	{
	public:
		Transmitter(Platform* platform, std::string name, const bool pulsed) noexcept
			: Radar(platform, std::move(name)), _pulsed(pulsed) {}

		~Transmitter() override = default;

		[[nodiscard]] RealType getPrf() const noexcept { return _prf; }
		[[nodiscard]] signal::RadarSignal* getSignal() const noexcept { return _signal; }
		[[nodiscard]] Transmitter* getDual() const noexcept { return _dual; }
		[[nodiscard]] bool getPulsed() const noexcept { return _pulsed; }

		[[nodiscard]] int getPulseCount() const noexcept;

		void setWave(signal::RadarSignal* pulse) noexcept { _signal = pulse; }
		void setDual(Transmitter* dual) noexcept { _dual = dual; }
		void setSignal(signal::RadarSignal* signal) noexcept { _signal = signal; }
		void setPulsed(const bool pulsed) noexcept { _pulsed = pulsed; }

		void setPulse(TransmitterPulse* pulse, int number) const;

		void setPrf(RealType mprf) noexcept;

	private:
		signal::RadarSignal* _signal = nullptr;
		RealType _prf = {};
		bool _pulsed;
		Transmitter* _dual = nullptr;
	};

	inline Transmitter* createMultipathDual(Transmitter* trans, const math::MultipathSurface* surf)
	{
		return createMultipathDualBase(trans, surf);
	}
}
