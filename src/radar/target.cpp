/**
 * @file target.cpp
 * @brief Defines classes for radar targets and their Radar Cross-Section (RCS) models.
 *
 * @authors David Young, Marc Brooker
 * @date 2007-06-11
 */

#include "target.h"

#include <cmath>
#include <optional>
#include <stdexcept>

#include "core/logging.h"
#include "math/geometry_ops.h"
#include "serial/libxml_wrapper.h"

using math::SVec3;

namespace
{
	/**
	 * @brief Load the target gain axis from an XML element.
	 *
	 * @param set The interpolation set to insert the samples into.
	 * @param axisXml The XML element containing the gain samples.
	 */
	void loadTargetGainAxis(const interp::InterpSet* set, const XmlElement& axisXml) noexcept
	{
		XmlElement tmp = axisXml.childElement("rcssample");
		while (tmp.isValid())
		{
			XmlElement angle_element = tmp.childElement("angle", 0);

			if (XmlElement gain_element = tmp.childElement("rcs", 0); angle_element.isValid() && gain_element.
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

namespace radar
{
	RealType IsoTarget::getRcs(SVec3& /*inAngle*/, SVec3& /*outAngle*/) const noexcept
	{
		return _model ? _rcs * _model->sampleModel() : _rcs;
	}

	FileTarget::FileTarget(Platform* platform, std::string name, const std::string& filename, const unsigned seed) :
		Target(platform, std::move(name), seed), _azi_samples(std::make_unique_for_overwrite<interp::InterpSet>()),
		_elev_samples(std::make_unique_for_overwrite<interp::InterpSet>())
	{
		XmlDocument doc;
		if (!doc.loadFile(filename)) { throw std::runtime_error("Could not load target description from " + filename); }

		const XmlElement root(doc.getRootElement());

		loadTargetGainAxis(_elev_samples.get(), root.childElement("elevation", 0));
		loadTargetGainAxis(_azi_samples.get(), root.childElement("azimuth", 0));
	}

	RealType FileTarget::getRcs(SVec3& inAngle, SVec3& outAngle) const
	{
		const SVec3 t_angle = inAngle + outAngle;

		const auto azi_value = _azi_samples->getValueAt(t_angle.azimuth / 2.0);

		if (const auto elev_value = _elev_samples->getValueAt(t_angle.elevation / 2.0); azi_value && elev_value)
		{
			const RealType rcs = std::sqrt(*azi_value * *elev_value);
			return _model ? rcs * _model->sampleModel() : rcs;
		}

		LOG(logging::Level::FATAL, "Could not get RCS value for target");
		throw std::runtime_error("Could not get RCS value for target");
	}
}
