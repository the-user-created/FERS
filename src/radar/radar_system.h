// radar_system.h
// Defines classes for receivers, transmitters and antennas
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

#ifndef RADAR_SYSTEM_H
#define RADAR_SYSTEM_H

#include <mutex>

#include "antenna_factory.h"
#include "config.h"
#include "core/object.h"
#include "simulation/response.h"
#include "timing/timing.h"

namespace rs
{
	class RadarSignal;
	class Response;
	class Antenna;
	class SVec3;
	class Vec3;
	class Timing;
	class Receiver;
	class Transmitter;
	class MultipathSurface;

	struct TransmitterPulse
	{
		RadarSignal* wave;
		RS_FLOAT time;
	};

	class Radar : public Object
	{
	public:
		Radar(const Platform* platform, const std::string& name) : Object(platform, name), _timing(nullptr),
		                                                           _antenna(nullptr), _attached(nullptr),
		                                                           _multipath_dual(false), _multipath_factor(0) {}

		~Radar() override = default;

		void setAntenna(const Antenna* ant)
		{
			!ant ? throw std::logic_error("[BUG] Transmitter's antenna set to null") : _antenna = ant;
		}

		[[nodiscard]] RS_FLOAT getGain(const SVec3& angle, const SVec3& refangle, const RS_FLOAT wavelength) const
		{
			return _antenna->getGain(angle, refangle, wavelength);
		}

		[[nodiscard]] virtual RS_FLOAT getNoiseTemperature(const SVec3& angle) const
		{
			return _antenna->getNoiseTemperature(angle);
		}

		void makeMonostatic(const Radar* recv)
		{
			_attached
				? throw std::runtime_error("[BUG] Attempted to attach second receiver to transmitter")
				: _attached = recv;
		}

		[[nodiscard]] const Radar* getAttached() const { return _attached; }

		[[nodiscard]] bool isMonostatic() const { return _attached; }

		void setTiming(const std::shared_ptr<Timing>& tim)
		{
			!tim ? throw std::runtime_error("[BUG] Radar timing source must not be set to NULL") : _timing = tim;
		}

		[[nodiscard]] std::shared_ptr<Timing> getTiming() const;

		[[nodiscard]] bool getMultipathDual() const { return _multipath_dual; }

		void setMultipathDual(RS_FLOAT reflect);

		[[nodiscard]] RS_FLOAT getMultipathFactor() const { return _multipath_factor; }

		[[nodiscard]] const Antenna* getAntenna() const { return _antenna; }

		void setAttached(const Radar* obj) { _attached = obj; }

	protected:
		std::shared_ptr<Timing> _timing;

	private:
		const Antenna* _antenna;
		const Radar* _attached;
		bool _multipath_dual;
		RS_FLOAT _multipath_factor;
	};

	class Transmitter final : public Radar
	{
	public:
		Transmitter(const Platform* platform, const std::string& name, const bool pulsed) : Radar(platform, name),
			_signal(nullptr), _pulsed(pulsed), _dual(nullptr) {}

		~Transmitter() override = default;

		void setWave(RadarSignal* pulse) { _signal = pulse; }

		[[nodiscard]] int getPulseCount() const;

		void getPulse(TransmitterPulse* pulse, int number) const;

		void setPrf(RS_FLOAT mprf);

		[[nodiscard]] Transmitter* getDual() const { return _dual; }

		[[nodiscard]] bool getPulsed() const { return _pulsed; }

		void setPulsed(const bool pulsed) { _pulsed = pulsed; }

		void setDual(Transmitter* dual) { _dual = dual; }

		[[nodiscard]] RS_FLOAT getPrf() const { return _prf; }

		[[nodiscard]] RadarSignal* getSignal() const { return _signal; }

		void setSignal(RadarSignal* signal) { _signal = signal; }

	private:
		RadarSignal* _signal;
		RS_FLOAT _prf{};
		bool _pulsed;
		Transmitter* _dual;
	};

	class Receiver final : public Radar
	{
	public:
		enum RecvFlag { FLAG_NODIRECT = 1, FLAG_NOPROPLOSS = 2 };

		explicit Receiver(const Platform* platform, const std::string& name = "defRecv") : Radar(platform, name),
			_noise_temperature(0), _dual(nullptr), _flags(0) {}

		~Receiver() override;

		void addResponse(Response* response);

		void clearResponses();

		void render();

		[[nodiscard]] RS_FLOAT getNoiseTemperature(const SVec3& angle) const override
		{
			return _noise_temperature + Radar::getNoiseTemperature(angle);
		}

		[[nodiscard]] RS_FLOAT getNoiseTemperature() const { return _noise_temperature; }

		void setNoiseTemperature(const RS_FLOAT temp)
		{
			temp < -std::numeric_limits<RS_FLOAT>::epsilon()
				? throw std::runtime_error("[BUG] Noise temperature must be positive")
				: _noise_temperature = temp;
		}

		void setWindowProperties(RS_FLOAT length, RS_FLOAT prf, RS_FLOAT skip);

		[[nodiscard]] int countResponses() const { return static_cast<int>(_responses.size()); }

		[[nodiscard]] int getWindowCount() const;

		[[nodiscard]] RS_FLOAT getWindowStart(int window) const;

		[[nodiscard]] RS_FLOAT getWindowLength() const { return _window_length; }

		[[nodiscard]] RS_FLOAT getWindowSkip() const { return _window_skip; }

		[[nodiscard]] RS_FLOAT getWindowPrf() const { return _window_prf; }

		void setFlag(const RecvFlag flag) { _flags |= flag; }

		[[nodiscard]] bool checkFlag(const RecvFlag flag) const { return _flags & flag; }

		[[nodiscard]] Receiver* getDual() const { return _dual; }

		void setDual(Receiver* dual) { _dual = dual; }

	private:
		std::vector<Response*> _responses;
		std::mutex _responses_mutex;
		RS_FLOAT _noise_temperature;
		RS_FLOAT _window_length{};
		RS_FLOAT _window_prf{};
		RS_FLOAT _window_skip{};
		Receiver* _dual;
		int _flags;
	};

	Receiver* createMultipathDual(Receiver* recv, const MultipathSurface* surf);

	Transmitter* createMultipathDual(Transmitter* trans, const MultipathSurface* surf);

	inline bool compareTimes(const Response* a, const Response* b) { return a->startTime() < b->startTime(); }
}

#endif
