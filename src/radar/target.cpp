// target.cpp
// Classes for targets and target RCS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#define TIXML_USE_STL

#include "target.h"

#include <cmath>
#include <tinyxml.h>

#include "interpolation/interpolation_set.h"
#include "noise/noise_generators.h"

using namespace rs;

RS_FLOAT getNodeFloat(const TiXmlHandle& node);

namespace
{
	void loadTargetGainAxis(const InterpSet* set, const TiXmlHandle& axisXml)
	{
		TiXmlHandle tmp = axisXml.ChildElement("rcssample", 0);
		for (int i = 0; tmp.Element() != nullptr; i++)
		{
			const RS_FLOAT angle = getNodeFloat(tmp.ChildElement("angle", 0));
			const RS_FLOAT gain = getNodeFloat(tmp.ChildElement("rcs", 0));
			set->insertSample(angle, gain);
			tmp = axisXml.ChildElement("rcssample", i);
		}
	}
}

// =====================================================================================================================
//
// RCS CHI SQUARE CLASS
//
// =====================================================================================================================

RcsChiSquare::RcsChiSquare(const RS_FLOAT k) : _gen(new GammaGenerator(k))
{
}


RcsChiSquare::~RcsChiSquare()
{
	delete _gen;
}

RS_FLOAT RcsChiSquare::sampleModel()
{
	return _gen->getSample();
}

// =====================================================================================================================
//
// FILE TARGET CLASS
//
// =====================================================================================================================

FileTarget::FileTarget(const Platform* platform, const std::string& name, const std::string& filename)
	: Target(platform, name), _azi_samples(new InterpSet()), _elev_samples(new InterpSet())
{
	loadRcsDescription(filename);
}

FileTarget::~FileTarget()
{
	delete _azi_samples;
	delete _elev_samples;
}


RS_FLOAT FileTarget::getRcs(SVec3& inAngle, SVec3& outAngle) const
{
	const SVec3 t_angle = inAngle + outAngle;
	const RS_FLOAT rcs = std::sqrt(
		_azi_samples->getValueAt(t_angle.azimuth / 2.0) * _elev_samples->getValueAt(t_angle.elevation / 2.0));
	return _model ? rcs * _model->sampleModel() : rcs;
}

void FileTarget::loadRcsDescription(const std::string& filename) const
{
	TiXmlDocument doc(filename.c_str());
	if (!doc.LoadFile())
	{
		throw std::runtime_error("[ERROR] Could not load target description from " + filename);
	}
	const TiXmlHandle root(doc.RootElement());
	TiXmlHandle tmp = root.ChildElement("elevation", 0);
	if (!tmp.Element())
	{
		throw std::runtime_error("[ERROR] Malformed XML in target description: No elevation pattern definition");
	}
	loadTargetGainAxis(_elev_samples, tmp);
	tmp = root.ChildElement("azimuth", 0);
	if (!tmp.Element())
	{
		throw std::runtime_error("[ERROR] Malformed XML in target description: No azimuth pattern definition");
	}
	loadTargetGainAxis(_azi_samples, tmp);
}
