// radar_system.h
// Defines classes for receivers, transmitters and antennas
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

#pragma once

#include <memory>                    // for unique_ptr, shared_ptr
#include <mutex>                     // for mutex
#include <string>                    // for string
#include <utility>                   // for move
#include <vector>                    // for vector

#include "config.h"                  // for RealType
#include "object.h"                  // for Object
#include "serial/response.h"  // for Response

namespace antenna
{
	class Antenna;
}

namespace math
{
	class MultipathSurface;
	class SVec3;
}

namespace signal
{
	class RadarSignal;
}

namespace timing
{
	class Timing;
}

namespace radar
{
	class Platform;

	struct TransmitterPulse
	{
		signal::RadarSignal* wave;
		RealType time;
	};

	class Radar : public Object
	{
	public:
		Radar(Platform* platform, std::string name) : Object(platform, std::move(name)) {}

		~Radar() override = default;

		[[nodiscard]] bool getMultipathDual() const { return _multipath_dual; }
		[[nodiscard]] const Radar* getAttached() const { return _attached; }
		[[nodiscard]] RealType getMultipathFactor() const { return _multipath_factor; }
		[[nodiscard]] const antenna::Antenna* getAntenna() const { return _antenna; }

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const;

		[[nodiscard]] virtual RealType getNoiseTemperature(const math::SVec3& angle) const;

		[[nodiscard]] std::shared_ptr<timing::Timing> getTiming() const;

		void setTiming(const std::shared_ptr<timing::Timing>& tim);

		void setAntenna(const antenna::Antenna* ant);

		void setMultipathDual(RealType reflect);

		void setAttached(const Radar* obj);

	protected:
		std::shared_ptr<timing::Timing> _timing;

	private:
		const antenna::Antenna* _antenna{nullptr};
		const Radar* _attached{nullptr};
		bool _multipath_dual{false};
		RealType _multipath_factor{0};
	};

	// Transmitter Class
	class Transmitter final : public Radar
	{
	public:
		Transmitter(Platform* platform, std::string name, const bool pulsed)
			: Radar(platform, std::move(name)), _pulsed(pulsed) {}

		~Transmitter() override = default;

		[[nodiscard]] RealType getPrf() const { return _prf; }
		[[nodiscard]] signal::RadarSignal* getSignal() const { return _signal; }
		[[nodiscard]] Transmitter* getDual() const { return _dual; }
		[[nodiscard]] bool getPulsed() const { return _pulsed; }

		[[nodiscard]] int getPulseCount() const;

		void setWave(signal::RadarSignal* pulse) { _signal = pulse; }
		void setDual(Transmitter* dual) { _dual = dual; }
		void setSignal(signal::RadarSignal* signal) { _signal = signal; }
		void setPulsed(const bool pulsed) { _pulsed = pulsed; }

		void setPulse(TransmitterPulse* pulse, int number) const;

		void setPrf(RealType mprf);

	private:
		signal::RadarSignal* _signal = nullptr;
		RealType _prf = {};
		bool _pulsed;
		Transmitter* _dual = nullptr;
	};

	// Receiver Class
	class Receiver final : public Radar
	{
	public:
		enum class RecvFlag { FLAG_NODIRECT = 1, FLAG_NOPROPLOSS = 2 };

		explicit Receiver(Platform* platform, std::string name = "defRecv") : Radar(platform, std::move(name)) {}

		~Receiver() override = default;

		void addResponse(std::unique_ptr<serial::Response> response);

		[[nodiscard]] bool checkFlag(RecvFlag flag) const { return _flags & static_cast<int>(flag); }

		void render();

		[[nodiscard]] RealType getNoiseTemperature() const { return _noise_temperature; }
		[[nodiscard]] RealType getWindowLength() const { return _window_length; }
		[[nodiscard]] RealType getWindowPrf() const { return _window_prf; }
		[[nodiscard]] RealType getWindowSkip() const { return _window_skip; }
		[[nodiscard]] Receiver* getDual() const { return _dual; }

		[[nodiscard]] RealType getNoiseTemperature(const math::SVec3& angle) const override;

		[[nodiscard]] RealType getWindowStart(int window) const;

		[[nodiscard]] int getWindowCount() const;

		[[nodiscard]] int getResponseCount() const;

		void setWindowProperties(RealType length, RealType prf, RealType skip);

		void setFlag(RecvFlag flag) { _flags |= static_cast<int>(flag); }
		void setDual(Receiver* dual) { _dual = dual; }

		void setNoiseTemperature(RealType temp);

	private:
		std::vector<std::unique_ptr<serial::Response>> _responses;
		mutable std::mutex _responses_mutex;
		RealType _noise_temperature = 0;
		RealType _window_length = 0;
		RealType _window_prf = 0;
		RealType _window_skip = 0;
		Receiver* _dual = nullptr;
		int _flags = 0;
	};

	// Factory functions
	Receiver* createMultipathDual(Receiver* recv, const math::MultipathSurface* surf);

	Transmitter* createMultipathDual(Transmitter* trans, const math::MultipathSurface* surf);

	// Compare function for responses
	inline bool compareTimes(const std::unique_ptr<serial::Response>& a, const std::unique_ptr<serial::Response>& b)
	{
		return a->startTime() < b->startTime();
	}
}
