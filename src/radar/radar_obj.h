// radar_obj.h
// Defines classes for receivers, transmitters and antennas
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

#pragma once

#include "object.h"

namespace timing
{
	class Timing;
}

namespace antenna
{
	class Antenna;
}

namespace radar
{
	class Platform;

	class Radar : public Object
	{
	public:
		Radar(Platform* platform, std::string name) noexcept : Object(platform, std::move(name)) {}

		~Radar() override = default;

		[[nodiscard]] bool getMultipathDual() const noexcept { return _multipath_dual; }
		[[nodiscard]] const Radar* getAttached() const noexcept { return _attached; }
		[[nodiscard]] RealType getMultipathFactor() const noexcept { return _multipath_factor; }
		[[nodiscard]] const antenna::Antenna* getAntenna() const noexcept { return _antenna; }

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const;

		[[nodiscard]] virtual RealType getNoiseTemperature(const math::SVec3& angle) const noexcept;

		[[nodiscard]] std::shared_ptr<timing::Timing> getTiming() const;

		void setTiming(const std::shared_ptr<timing::Timing>& tim);

		void setAntenna(const antenna::Antenna* ant);

		void setMultipathDual(RealType reflect) noexcept;

		void setAttached(const Radar* obj);

	protected:
		std::shared_ptr<timing::Timing> _timing;

	private:
		const antenna::Antenna* _antenna{nullptr};
		const Radar* _attached{nullptr};
		bool _multipath_dual{false};
		RealType _multipath_factor{0};
	};

	template <typename T>
	T* createMultipathDualBase(T* obj, const math::MultipathSurface* surf);
}
