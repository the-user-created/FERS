/**
 * @file antenna_factory.cpp
 * @brief Implementation of the Antenna class and its derived classes.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-07-20
 */

#include "antenna_factory.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <optional>
#include <stdexcept>

#include "core/logging.h"
#include "core/portable_utils.h"
#include "math/geometry_ops.h"
#include "serial/libxml_wrapper.h"

using logging::Level;
using math::SVec3;
using math::Vec3;

namespace
{
	/**
	 * @brief Compute the sinc function.
	 *
	 * @param theta The angle for which to compute the sinc function.
	 * @return The value of the sinc function at the given angle theta.
	 */
	RealType sinc(const RealType theta) noexcept { return std::sin(theta) / (theta + EPSILON); }

	/**
	 * @brief Compute the Bessel function of the first kind.
	 *
	 * @param x The value for which to compute the Bessel function.
	 * @return The value of the Bessel function of the first kind at the given value x.
	 */
	RealType j1C(const RealType x) noexcept { return x == 0 ? 1.0 : core::besselJ1(x) / x; }

	/**
	 * @brief Load antenna gain axis data from an XML element.
	 *
	 * @param set The interpolation set to store the gain axis data.
	 * @param axisXml The XML element containing the gain axis data.
	 */
	void loadAntennaGainAxis(const interp::InterpSet* set, const XmlElement& axisXml) noexcept
	{
		XmlElement tmp = axisXml.childElement("gainsample");
		while (tmp.isValid())
		{
			XmlElement angle_element = tmp.childElement("angle", 0);

			if (XmlElement gain_element = tmp.childElement("gain", 0); angle_element.isValid() && gain_element.
				isValid())
			{
				const RealType angle = std::stof(angle_element.getText());
				const RealType gain = std::stof(gain_element.getText());
				set->insertSample(angle, gain);
			}

			tmp = XmlElement(tmp.getNode()->next);
		}
	}
}

namespace antenna
{
	void Antenna::setEfficiencyFactor(const RealType loss) noexcept
	{
		if (loss > 1) { LOG(Level::INFO, "Using greater than unity antenna efficiency."); }
		_loss_factor = loss;
	}

	RealType Antenna::getAngle(const SVec3& angle, const SVec3& refangle) noexcept
	{
		SVec3 normangle(angle);
		normangle.length = 1;
		return std::acos(dotProduct(Vec3(normangle), Vec3(refangle)));
	}

	RealType Gaussian::getGain(const SVec3& angle, const SVec3& refangle, RealType /*wavelength*/) const noexcept
	{
		const SVec3 a = angle - refangle;
		return std::exp(-a.azimuth * a.azimuth * _azscale) * std::exp(-a.elevation * a.elevation * _elscale);
	}

	RealType Sinc::getGain(const SVec3& angle, const SVec3& refangle, RealType /*wavelength*/) const noexcept
	{
		const RealType theta = getAngle(angle, refangle);
		const ComplexType complex_sinc(sinc(_beta * theta), 0.0);
		const ComplexType complex_gain = _alpha * std::pow(complex_sinc, ComplexType(_gamma, 0.0)) *
			getEfficiencyFactor();
		return std::abs(complex_gain);
	}

	RealType SquareHorn::getGain(const SVec3& angle, const SVec3& refangle, const RealType wavelength) const noexcept
	{
		const RealType ge = 4 * PI * std::pow(_dimension, 2) / std::pow(wavelength, 2);
		const RealType x = PI * _dimension * std::sin(getAngle(angle, refangle)) / wavelength;
		return ge * std::pow(sinc(x), 2) * getEfficiencyFactor();
	}

	RealType Parabolic::getGain(const SVec3& angle, const SVec3& refangle, const RealType wavelength) const noexcept
	{
		const RealType ge = std::pow(PI * _diameter / wavelength, 2);
		const RealType x = PI * _diameter * std::sin(getAngle(angle, refangle)) / wavelength;
		return ge * std::pow(2 * j1C(x), 2) * getEfficiencyFactor();
	}

	RealType XmlAntenna::getGain(const SVec3& angle, const SVec3& refangle, RealType /*wavelength*/) const
	{
		const SVec3 delta_angle = angle - refangle;

		const std::optional<RealType> azi_value = _azi_samples->getValueAt(std::abs(delta_angle.azimuth));

		if (const std::optional<RealType> elev_value = _elev_samples->getValueAt(std::abs(delta_angle.elevation));
			azi_value && elev_value) { return *azi_value * *elev_value * _max_gain * getEfficiencyFactor(); }

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
