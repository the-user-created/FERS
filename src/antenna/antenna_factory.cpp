// antenna_factory.cpp
// Implementation of Antenna Class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 July 2006

#define TIXML_USE_STL

#include "antenna_factory.h"

#include <tinyxml.h>

#include "core/logging.h"
#include "core/portable_utils.h"
#include "math_utils/geometry_ops.h"

using logging::Level;
using math::SVec3;
using math::Vec3;

RealType getNodeFloat(const TiXmlHandle& node);

namespace
{
	constexpr RealType EPSILON = std::numeric_limits<RealType>::epsilon();

	RealType sinc(const RealType theta) { return std::sin(theta) / (theta + EPSILON); }

	RealType j1C(const RealType x) { return x == 0 ? 1.0 : core::besselJ1(x) / x; }

	void loadAntennaGainAxis(const interp::InterpSet* set, const TiXmlHandle& axisXml)
	{
		TiXmlHandle tmp = axisXml.ChildElement("gainsample", 0);
		for (int i = 0; tmp.Element() != nullptr; i++)
		{
			const RealType angle = getNodeFloat(tmp.ChildElement("angle", 0));
			const RealType gain = getNodeFloat(tmp.ChildElement("gain", 0));
			set->insertSample(angle, gain);
			tmp = axisXml.ChildElement("gainsample", i);
		}
	}
}

namespace antenna
{
	constexpr RealType PI = std::numbers::pi; // Constant for π

	// =================================================================================================================
	//
	// ANTENNA CLASS
	//
	// =================================================================================================================

	void Antenna::setEfficiencyFactor(const RealType loss)
	{
		if (loss > 1) { LOG(Level::INFO, "Using greater than unity antenna efficiency."); }
		_loss_factor = loss;
	}

	RealType Antenna::getAngle(const SVec3& angle, const SVec3& refangle)
	{
		SVec3 normangle(angle);
		normangle.length = 1;
		return std::acos(dotProduct(Vec3(normangle), Vec3(refangle)));
	}

	// =================================================================================================================
	//
	// GAUSSIAN CLASS
	//
	// =================================================================================================================

	RealType Gaussian::getGain(const SVec3& angle, const SVec3& refangle, RealType wavelength) const
	{
		const SVec3 a = angle - refangle;
		return std::exp(-a.azimuth * a.azimuth * _azscale) * std::exp(-a.elevation * a.elevation * _elscale);
	}

	// =================================================================================================================
	//
	// SINC CLASS
	//
	// =================================================================================================================

	RealType Sinc::getGain(const SVec3& angle, const SVec3& refangle, RealType wavelength) const
	{
		const RealType theta = getAngle(angle, refangle);
		const ComplexType complex_sinc(sinc(_beta * theta), 0.0);
		const ComplexType complex_gain = _alpha * std::pow(complex_sinc, ComplexType(_gamma, 0.0)) *
			getEfficiencyFactor();
		return std::abs(complex_gain);
	}

	// =================================================================================================================
	//
	// SQUAREHORN CLASS
	//
	// =================================================================================================================

	RealType SquareHorn::getGain(const SVec3& angle, const SVec3& refangle, const RealType wavelength) const
	{
		const RealType ge = 4 * PI * std::pow(_dimension, 2) / std::pow(wavelength, 2);
		const RealType x = PI * _dimension * std::sin(getAngle(angle, refangle)) / wavelength;
		return ge * std::pow(sinc(x), 2) * getEfficiencyFactor();
	}

	// =================================================================================================================
	//
	// PARABOLIC REFLECTOR CLASS
	//
	// =================================================================================================================

	RealType ParabolicReflector::getGain(const SVec3& angle, const SVec3& refangle, const RealType wavelength) const
	{
		const RealType ge = std::pow(PI * _diameter / wavelength, 2);
		const RealType x = PI * _diameter * std::sin(getAngle(angle, refangle)) / wavelength;
		return ge * std::pow(2 * j1C(x), 2) * getEfficiencyFactor();
	}

	// =================================================================================================================
	//
	// XML ANTENNA CLASS
	//
	// =================================================================================================================

	RealType XmlAntenna::getGain(const SVec3& angle, const SVec3& refangle, RealType wavelength) const
	{
		const SVec3 delta_angle = angle - refangle;

		// Get the optional values from _azi_samples and _elev_samples
		const std::optional<RealType> azi_value = _azi_samples->getValueAt(std::abs(delta_angle.azimuth));

		// Check if both optional values are valid
		if (const std::optional<RealType> elev_value = _elev_samples->getValueAt(std::abs(delta_angle.elevation));
			azi_value && elev_value)
		{
			// Safely unwrap the optional values and multiply them
			return *azi_value * *elev_value * _max_gain * getEfficiencyFactor();
		}

		LOG(Level::FATAL, "Could not get antenna gain value");
		throw std::runtime_error("Could not get antenna gain value");
	}

	void XmlAntenna::loadAntennaDescription(const std::string_view filename)
	{
		TiXmlDocument doc((filename.data()));
		if (!doc.LoadFile())
		{
			LOG(Level::FATAL, "Could not load antenna description {}", filename.data());
			throw std::runtime_error("Could not load antenna description");
		}
		const TiXmlHandle root(doc.RootElement());
		loadAntennaGainAxis(_elev_samples.get(), root.ChildElement("elevation", 0));
		loadAntennaGainAxis(_azi_samples.get(), root.ChildElement("azimuth", 0));
		_max_gain = std::max(_azi_samples->getMax(), _elev_samples->getMax());
		_elev_samples->divide(_max_gain);
		_azi_samples->divide(_max_gain);
	}
}
