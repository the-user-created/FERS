//rstarget.cpp - Classes for targets and target RCS
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//11 June 2007

#define TIXML_USE_STL //Tell tinyxml to use the STL instead of it's own string class

#include "rstarget.h"

#include <cmath>
#include <tinyxml.h>

#include "rsinterp.h"
#include "rsnoise.h"

using namespace rs;

//One of the xml utility functions from xmlimport.cpp
rsFloat getNodeFloat(const TiXmlHandle& node);

//
// RCSModel Implementation
//

/// Destructor
RcsModel::~RcsModel()
{
}

//
// RCSConst Implementation
//

/// Return a constant RCS
rsFloat RcsConst::sampleModel()
{
	return 1.0;
}

/// Destructor
RcsConst::~RcsConst()
{
}

//
// RCSChiSquare Implementation

/// Constructor
RcsChiSquare::RcsChiSquare(const rsFloat k)
{
	_gen = new GammaGenerator(k);
}

/// Destructor
RcsChiSquare::~RcsChiSquare()
{
	delete _gen;
}

/// Get an RCS based on the Swerling II model and the mean RCS
rsFloat RcsChiSquare::sampleModel()
{
	return _gen->getSample();
}

//
// Target Implementation
//

//Default constructor for Target object
Target::Target(const Platform* platform, const std::string& name):
	Object(platform, name)
{
	_model = nullptr;
}

//Default destructor for Target object
Target::~Target()
{
	delete _model;
}

/// Get the target polarization matrix
PsMatrix Target::getPolarization() const
{
	return _psm;
}

/// Set the target polarization matrix
void Target::setPolarization(const PsMatrix& in)
{
	_psm = in;
}

/// Set the target fluctuation model
void Target::setFluctuationModel(RcsModel* in)
{
	_model = in;
}

//
// IsoTarget Implementation
//

/// Constructor
IsoTarget::IsoTarget(const Platform* platform, const std::string& name, const rsFloat rcs):
	Target(platform, name),
	_rcs(rcs)
{
}

/// Destructor
IsoTarget::~IsoTarget()
{
}

/// Return the RCS at the given angle
rsFloat IsoTarget::getRcs(SVec3& inAngle, SVec3& outAngle) const
{
	if (_model)
	{
		return _rcs * _model->sampleModel();
	}
	else
	{
		return _rcs;
	}
}

//
// FileTarget Implementation
//

/// Constructor
FileTarget::FileTarget(const Platform* platform, const std::string& name, const std::string& filename):
	Target(platform, name)
{
	//Create the objects for azimuth and elevation interpolation
	_azi_samples = new InterpSet();
	_elev_samples = new InterpSet();
	//Load the data from the description file into the interpolation objects
	loadRcsDescription(filename);
}

/// Destructor
FileTarget::~FileTarget()
{
	delete _azi_samples;
	delete _elev_samples;
}


/// Return the RCS at the given angle
rsFloat FileTarget::getRcs(SVec3& inAngle, SVec3& outAngle) const
{
	//Currently uses a half angle approximation, this needs to be improved
	const SVec3 t_angle = inAngle + outAngle;
	const rsFloat rcs = std::sqrt(
		_azi_samples->value(t_angle.azimuth / 2.0) * _elev_samples->value(t_angle.elevation / 2.0));
	if (_model)
	{
		return rcs * _model->sampleModel();
	}
	else
	{
		return rcs;
	}
}

namespace
{
	//Load samples of gain along an axis (not a member of FileAntenna)
	void loadTargetGainAxis(const InterpSet* set, const TiXmlHandle& axisXml)
	{
		//Step through the XML file and load all the gain samples
		TiXmlHandle tmp = axisXml.ChildElement("rcssample", 0);
		for (int i = 0; tmp.Element() != nullptr; i++)
		{
			//Load the angle of the gain sample
			TiXmlHandle angle_xml = tmp.ChildElement("angle", 0);
			if (!angle_xml.Element())
			{
				throw std::runtime_error("[ERROR] Misformed XML in target description: No angle in rcssample");
			}
			const rsFloat angle = getNodeFloat(angle_xml);
			//Load the gain of the gain sample
			TiXmlHandle gain_xml = tmp.ChildElement("rcs", 0);
			if (!gain_xml.Element())
			{
				throw std::runtime_error("[ERROR] Misformed XML in target description: No rcs in rcssample");
			}
			const rsFloat gain = getNodeFloat(gain_xml);
			//Load the values into the interpolation table
			set->insertSample(angle, gain);
			//Get the next gainsample in the file
			tmp = axisXml.ChildElement("rcssample", i);
		}
	}
} //End of Anon. namespace

///Load data from the RCS description file
void FileTarget::loadRcsDescription(const std::string& filename) const
{
	TiXmlDocument doc(filename.c_str());
	//Check the document was loaded correctly
	if (!doc.LoadFile())
	{
		throw std::runtime_error("[ERROR] Could not load target description from " + filename);
	}
	//Get the XML root node
	const TiXmlHandle root(doc.RootElement());
	//Load the gain samples along the elevation axis
	TiXmlHandle tmp = root.ChildElement("elevation", 0);
	if (!tmp.Element())
	{
		throw std::runtime_error("[ERROR] Malformed XML in target description: No elevation pattern definition");
	}
	loadTargetGainAxis(_elev_samples, tmp);
	//Load the gain samples along the azimuth axis
	tmp = root.ChildElement("azimuth", 0);
	if (!tmp.Element())
	{
		throw std::runtime_error("[ERROR] Malformed XML in target description: No azimuth pattern definition");
	}
	loadTargetGainAxis(_azi_samples, tmp);
}

//
// Functions for creating objects of various target types
//

/// Create an isometric radiator target
Target* rs::createIsoTarget(const Platform* platform, const std::string& name, const rsFloat rcs)
{
	return new IsoTarget(platform, name, rcs);
}

/// Create a target, loading the RCS pattern from a file
Target* rs::createFileTarget(const Platform* platform, const std::string& name, const std::string& filename)
{
	return new FileTarget(platform, name, filename);
}
