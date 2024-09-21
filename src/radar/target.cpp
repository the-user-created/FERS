// target.cpp
// Classes for targets and target RCS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#define TIXML_USE_STL

#include "target.h"

#include <tinyxml.h>

#include "core/logging.h"

using math::SVec3;

RealType getNodeFloat(const TiXmlHandle& node);

namespace
{
	void loadTargetGainAxis(const interp::InterpSet* set, const TiXmlHandle& axisXml)
	{
		TiXmlHandle tmp = axisXml.ChildElement("rcssample", 0);
		for (int i = 0; tmp.Element() != nullptr; i++)
		{
			const RealType angle = getNodeFloat(tmp.ChildElement("angle", 0));
			const RealType gain = getNodeFloat(tmp.ChildElement("rcs", 0));
			set->insertSample(angle, gain);
			tmp = axisXml.ChildElement("rcssample", i);
		}
	}
}

namespace radar
{
	// =================================================================================================================
	//
	// FILE TARGET CLASS
	//
	// =================================================================================================================

	RealType FileTarget::getRcs(SVec3& inAngle, SVec3& outAngle) const
	{
		const SVec3 t_angle = inAngle + outAngle;

		// Get the optional values from _azi_samples and _elev_samples
		const std::optional<RealType> azi_value = _azi_samples->getValueAt(t_angle.azimuth / 2.0);

		// Check if both values are valid
		if (const std::optional<RealType> elev_value = _elev_samples->getValueAt(t_angle.elevation / 2.0); azi_value &&
			elev_value)
		{
			// Safely unwrap the optional values and multiply them
			const RealType rcs = std::sqrt((*azi_value) * (*elev_value));

			// Return rcs, adjusted by _model->sampleModel() if _model is valid
			return _model ? rcs * _model->sampleModel() : rcs;
		}

		LOG(logging::Level::FATAL, "Could not get RCS value for target");
		throw std::runtime_error("Could not get RCS value for target");
	}

	void FileTarget::loadRcsDescription(const std::string& filename) const
	{
		TiXmlDocument doc(filename.c_str());
		if (!doc.LoadFile()) { throw std::runtime_error("Could not load target description from " + filename); }
		const TiXmlHandle root(doc.RootElement());
		TiXmlHandle tmp = root.ChildElement("elevation", 0);
		if (!tmp.Element())
		{
			throw std::runtime_error("Malformed XML in target description: No elevation pattern definition");
		}
		loadTargetGainAxis(_elev_samples.get(), tmp);
		tmp = root.ChildElement("azimuth", 0);
		if (!tmp.Element())
		{
			throw std::runtime_error("Malformed XML in target description: No azimuth pattern definition");
		}
		loadTargetGainAxis(_azi_samples.get(), tmp);
	}
}
