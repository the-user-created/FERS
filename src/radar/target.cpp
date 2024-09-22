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
		auto insert_sample_from_xml = [&](const int i)
		{
			const TiXmlHandle sample_xml = axisXml.ChildElement("rcssample", i);
			if (!sample_xml.Element()) { return false; }

			const RealType angle = getNodeFloat(sample_xml.ChildElement("angle", 0));
			const RealType gain = getNodeFloat(sample_xml.ChildElement("rcs", 0));
			set->insertSample(angle, gain);
			return true;
		};

		for (int i = 0; insert_sample_from_xml(i); ++i) {}
	}
}

namespace radar
{
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

	void FileTarget::loadRcsDescription(const std::string& filename) const
	{
		TiXmlDocument doc(filename.c_str());
		if (!doc.LoadFile()) { throw std::runtime_error("Could not load target description from " + filename); }

		const TiXmlHandle root(doc.RootElement());
		const auto elev_xml = root.ChildElement("elevation", 0);
		const auto azi_xml = root.ChildElement("azimuth", 0);

		if (!elev_xml.Element())
		{
			throw std::runtime_error("Malformed XML in target description: No elevation pattern definition");
		}
		loadTargetGainAxis(_elev_samples.get(), elev_xml);

		if (!azi_xml.Element())
		{
			throw std::runtime_error("Malformed XML in target description: No azimuth pattern definition");
		}
		loadTargetGainAxis(_azi_samples.get(), azi_xml);
	}
}
