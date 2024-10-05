// antenna_factory.cpp
// Implementation of Antenna Class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 July 2006

#define TIXML_USE_STL

#include "antenna_factory.h"

#include <algorithm>                  // for max
#include <cmath>                      // for pow, sin, exp, acos
#include <complex>                    // for operator*, abs, complex, pow
#include <limits>                     // for numeric_limits
#include <optional>                   // for optional
#include <stdexcept>                  // for runtime_error
#include <bits/std_abs.h>             // for abs

#include "core/logging.h"             // for log, LOG, Level
#include "core/portable_utils.h"      // for besselJ1
#include "math_utils/geometry_ops.h"  // for SVec3, Vec3, operator-, dotProduct
#include "serialization/libxml_wrapper.h"

using logging::Level;
using math::SVec3;
using math::Vec3;

namespace
{
	RealType sinc(const RealType theta) { return std::sin(theta) / (theta + EPSILON); }

	RealType j1C(const RealType x) { return x == 0 ? 1.0 : core::besselJ1(x) / x; }

	void loadAntennaGainAxis(const interp::InterpSet* set, const XmlElement& axisXml)
	{
		XmlElement tmp = axisXml.childElement("gainsample"); // Get the first gainsample
		while (tmp.isValid()) // Continue while the element is valid
		{
			XmlElement angle_element = tmp.childElement("angle", 0);

			if (XmlElement gain_element = tmp.childElement("gain", 0); angle_element.isValid() && gain_element.
				isValid())
			{
				const RealType angle = std::stof(angle_element.getText());
				const RealType gain = std::stof(gain_element.getText());
				set->insertSample(angle, gain);
			}

			// Move to the next sibling gainsample
			tmp = XmlElement(tmp.getNode()->next);
		}
	}
}

namespace antenna
{
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

	RealType Parabolic::getGain(const SVec3& angle, const SVec3& refangle, const RealType wavelength) const
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
		XmlDocument doc;
		if (!doc.loadFile(std::string(filename)))
		{
			LOG(Level::FATAL, "Could not load antenna description {}", filename.data());
			throw std::runtime_error("Could not load antenna description");
		}

		const XmlElement root(doc.getRootElement());
		loadAntennaGainAxis(_elev_samples.get(), root.childElement("elevation", 0));
		loadAntennaGainAxis(_azi_samples.get(), root.childElement("azimuth", 0));

		_max_gain = std::max(_azi_samples->getMax(), _elev_samples->getMax());
		_elev_samples->divide(_max_gain);
		_azi_samples->divide(_max_gain);
	}
}
