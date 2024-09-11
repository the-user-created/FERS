//rsantenna.cpp
//Implementation of Antenna Class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//20 July 2006

#define TIXML_USE_STL //Tell tinyxml to use the STL instead of it's own string class

#include "rsantenna.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tinyxml.h>
#include <utility>

#include "rsdebug.h"
#include "rsinterp.h"
#include "rspattern.h"
#include "rsportable.h"
#include "rsradarwaveform.h"

using namespace rs;
using namespace rs_antenna;

//One of the xml utility functions from xmlimport.cpp
RS_FLOAT getNodeFloat(const TiXmlHandle& node);

namespace
{
	RS_FLOAT sinc(const RS_FLOAT theta)
	{
		return std::sin(theta) / (theta + std::numeric_limits<RS_FLOAT>::epsilon());
	}

	//Return the first order, first kind bessel function of x, divided by x
	RS_FLOAT j1C(const RS_FLOAT x)
	{
		if (x == 0)
		{
			return 1;
		}
		return rs_portable::besselJ1(x) / x;
	}
}

//Default constructor for the antenna
Antenna::Antenna(std::string  name):
	_loss_factor(1), //Antenna efficiency default is unity
	_name(std::move(name))
{
}

//Antenna destructor
Antenna::~Antenna() = default;

//Set the efficiency factor of the antenna
void Antenna::setEfficiencyFactor(const RS_FLOAT loss)
{
	if (loss > 1)
	{
		rs_debug::printf(rs_debug::RS_IMPORTANT,
		                 "Using greater than unity antenna efficiency, results might be inconsistent with reality.\n");
	}
	_loss_factor = loss;
}

//Get the efficiency factor
RS_FLOAT Antenna::getEfficiencyFactor() const
{
	return _loss_factor;
}

//Return the name of the antenna
std::string Antenna::getName() const
{
	return _name;
}

// Get the angle (in radians) off boresight
RS_FLOAT Antenna::getAngle(const SVec3& angle, const SVec3& refangle)
{
	//Get the angle off boresight
	SVec3 normangle(angle);
	normangle.length = 1;
	const Vec3 cangle(normangle);
	const Vec3 ref(refangle);
	return std::acos(dotProduct(cangle, ref));
}

/// Get the noise temperature of the antenna in a particular direction
RS_FLOAT Antenna::getNoiseTemperature(const SVec3& angle) const
{
	return 0; //TODO: Antenna noise temperature calculation
}

//
//Isotropic Implementation
//

//Default constructor
Isotropic::Isotropic(const std::string& name):
	Antenna(name)
{
}

//Default destructor
Isotropic::~Isotropic() = default;

//Return the gain of the antenna
RS_FLOAT Isotropic::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	return getEfficiencyFactor();
}

//
// Gaussian Implementation
//


/// Constructor
Gaussian::Gaussian(const std::string& name, const RS_FLOAT azscale, const RS_FLOAT elscale):
	Antenna(name),
	_azscale(azscale),
	_elscale(elscale)
{
}

/// Destructor
Gaussian::~Gaussian() = default;

/// Get the gain at an angle
RS_FLOAT Gaussian::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	const SVec3 a = angle - refangle;
	const RS_FLOAT azfactor = std::exp(-a.azimuth * a.azimuth * _azscale);
	const RS_FLOAT elfactor = std::exp(-a.elevation * a.elevation * _elscale);
	return azfactor * elfactor;
}


//
//Sinc Implemetation
//

//Constructor
Sinc::Sinc(const std::string& name, const RS_FLOAT alpha, const RS_FLOAT beta, const RS_FLOAT gamma):
	Antenna(name),
	_alpha(alpha),
	_beta(beta),
	_gamma(gamma)
{
}

//Default destructor
Sinc::~Sinc() = default;

// Return the gain of the antenna at an angle
RS_FLOAT Sinc::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	//Get the angle off boresight
	const RS_FLOAT theta = getAngle(angle, refangle);

	//FIX 2015/02/18 CA Tong:
	//std::pow<double> returns NaN for negative bases with certain fractional indices as they create an uneven
	//root which is, of course, a complex number. Inputs therefore need to be cast to complex numbers before the
	//calculation as this will return complex number. Then return the magnitude as the beam gain.

	const RsComplex complex_sinc(sinc(_beta * theta), 0.0);
	const RsComplex complex_gamma(_gamma, 0.0);

	//See "Sinc Pattern" in doc/equations.tex for equation used here
	const RsComplex complex_gain = _alpha * std::pow(complex_sinc, complex_gamma) * getEfficiencyFactor();

	//rsDebug::printf(rsDebug::RS_IMPORTANT, "Theta = %f Gain = %f, %f\n", theta, complexGain.real(), complexGain.imag());

	return std::abs(complex_gain);
}

//
// SquareHorn Implementation
//

//Constructor
SquareHorn::SquareHorn(const std::string& name, const RS_FLOAT dimension):
	Antenna(name),
	_dimension(dimension)
{
}

//Destructor
SquareHorn::~SquareHorn() = default;

//Return the gain of the antenna
//See doc/equations.tex for details
RS_FLOAT SquareHorn::getGain(const SVec3& angle, const SVec3& refangle, const RS_FLOAT wavelength) const
{
	const RS_FLOAT ge = 4 * M_PI * _dimension * _dimension / (wavelength * wavelength);
	const RS_FLOAT x = M_PI * _dimension * std::sin(getAngle(angle, refangle)) / wavelength;
	const RS_FLOAT gain = ge * std::pow(sinc(x), 2);
	return gain * getEfficiencyFactor();
}


//
// Parabolic Dish Antenna
//

// Constructor
ParabolicReflector::ParabolicReflector(const std::string& name, const RS_FLOAT diameter):
	Antenna(name),
	_diameter(diameter)
{
}

//Destructor
ParabolicReflector::~ParabolicReflector() = default;

//Return the gain of the antenna
//See doc/equations.tex for details
RS_FLOAT ParabolicReflector::getGain(const SVec3& angle, const SVec3& refangle, const RS_FLOAT wavelength) const
{
	const RS_FLOAT ge = std::pow(M_PI * _diameter / wavelength, 2);
	const RS_FLOAT x = M_PI * _diameter * std::sin(getAngle(angle, refangle)) / wavelength;
	const RS_FLOAT gain = ge * std::pow(2 * j1C(x), 2);
	return gain * getEfficiencyFactor();
}

//
// FileAntenna implementation
//

/// Constructor
FileAntenna::FileAntenna(const std::string& name, const std::string& filename):
	Antenna(name)
{
	_pattern = new Pattern(filename);
}

/// Default destructor
FileAntenna::~FileAntenna()
{
	delete _pattern;
}

/// Get the gain at an angle
RS_FLOAT FileAntenna::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	const SVec3& a1 = angle;
	const SVec3& a2 = refangle;
	const SVec3 in_angle = a1 - a2;
	//  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "az: %g el: %g\t az2: %g el2: %g\t az3: %g el3: %g\n", angle.azimuth, angle.elevation, refangle.azimuth, refangle.elevation, in_angle.azimuth, refangle.elevation);
	return _pattern->getGain(in_angle) * getEfficiencyFactor();
}

//
// Antenna with pattern loaded from an XML file
//

// Constructor
XmlAntenna::XmlAntenna(const std::string& name, const std::string& filename):
	Antenna(name)
{
	// Classes to interpolate across elevation and azimuth
	_azi_samples = new InterpSet();
	_elev_samples = new InterpSet();
	//Load the XML antenna description data
	loadAntennaDescription(filename);
}

//Destructor
XmlAntenna::~XmlAntenna()
{
	// Clean up the interpolation classes
	delete _azi_samples;
	delete _elev_samples;
}

//Return the gain of the antenna
//See doc/equations.tex for details
RS_FLOAT XmlAntenna::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	const SVec3 t_angle = angle - refangle;
	const RS_FLOAT azi_gain = _azi_samples->value(std::fabs(t_angle.azimuth));
	const RS_FLOAT elev_gain = _elev_samples->value(std::fabs(t_angle.elevation));
	return azi_gain * elev_gain * _max_gain * getEfficiencyFactor();
}

namespace
{
	//Load samples of gain along an axis (not a member of XMLAntenna)
	void loadAntennaGainAxis(const InterpSet* set, const TiXmlHandle& axisXml)
	{
		//Step through the XML file and load all the gain samples
		TiXmlHandle tmp = axisXml.ChildElement("gainsample", 0);
		for (int i = 0; tmp.Element() != nullptr; i++)
		{
			//Load the angle of the gain sample
			TiXmlHandle angle_xml = tmp.ChildElement("angle", 0);
			if (!angle_xml.Element())
			{
				throw std::runtime_error("[ERROR] Misformed XML in antenna description: No angle in gainsample");
			}
			const RS_FLOAT angle = getNodeFloat(angle_xml);
			//Load the gain of the gain sample
			TiXmlHandle gain_xml = tmp.ChildElement("gain", 0);
			if (!gain_xml.Element())
			{
				throw std::runtime_error("[ERROR] Misformed XML in antenna description: No gain in gainsample");
			}
			const RS_FLOAT gain = getNodeFloat(gain_xml);
			//Load the values into the interpolation table
			set->insertSample(angle, gain);
			//Get the next gainsample in the file
			tmp = axisXml.ChildElement("gainsample", i);
		}
	}
}

//Load the antenna description file
void XmlAntenna::loadAntennaDescription(const std::string& filename)
{
	TiXmlDocument doc(filename.c_str());
	//Check the document was loaded correctly
	if (!doc.LoadFile())
	{
		throw std::runtime_error("[ERROR] Could not load antenna description " + filename);
	}
	//Get the XML root node
	const TiXmlHandle root(doc.RootElement());
	//Load the gain samples along the elevation axis
	TiXmlHandle tmp = root.ChildElement("elevation", 0);
	if (!tmp.Element())
	{
		throw std::runtime_error("[ERROR] Malformed XML in antenna description: No elevation pattern definition");
	}
	loadAntennaGainAxis(_elev_samples, tmp);
	//Load the gain samples along the azimuth axis
	tmp = root.ChildElement("azimuth", 0);
	if (!tmp.Element())
	{
		throw std::runtime_error("[ERROR] Malformed XML in antenna description: No azimuth pattern definition");
	}
	loadAntennaGainAxis(_azi_samples, tmp);
	// Normalize the antenna patterns and calculate the max gain
	_max_gain = std::max(_azi_samples->max(), _elev_samples->max());
	_elev_samples->divide(_max_gain);
	_azi_samples->divide(_max_gain);
}

//
// Antenna with gain pattern calculated by a Python program
//

// Constructor
PythonAntenna::PythonAntenna(const std::string& name, const std::string& module, const std::string& function):
	Antenna(name),
	_py_antenna(module, function)
{
}

//Destructor
PythonAntenna::~PythonAntenna() = default;

//Return the gain of the antenna
RS_FLOAT PythonAntenna::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	const SVec3 angle_bore = angle - refangle; //Calculate the angle off boresight
	const RS_FLOAT gain = _py_antenna.getGain(angle_bore);
	return gain * getEfficiencyFactor();
}


//
// Functions to create Antenna objects with a variety of properties
//

//Create an isotropic antenna with the specified name
Antenna* rs::createIsotropicAntenna(const std::string& name)
{
	auto* iso = new Isotropic(name);
	return iso;
}

//Create a Sinc pattern antenna with the specified name, alpha and beta
Antenna* rs::createSincAntenna(const std::string& name, const RS_FLOAT alpha, const RS_FLOAT beta, const RS_FLOAT gamma)
{
	const auto sinc = new Sinc(name, alpha, beta, gamma);
	return sinc;
}

//Create a Gaussian pattern antenna
Antenna* rs::createGaussianAntenna(const std::string& name, const RS_FLOAT azscale, const RS_FLOAT elscale)
{
	auto* gau = new Gaussian(name, azscale, elscale);
	return gau;
}

//Create a square horn antenna
Antenna* rs::createHornAntenna(const std::string& name, const RS_FLOAT dimension)
{
	auto* sq = new SquareHorn(name, dimension);
	return sq;
}

//Create a parabolic reflector antenna
Antenna* rs::createParabolicAntenna(const std::string& name, const RS_FLOAT diameter)
{
	auto* pd = new ParabolicReflector(name, diameter);
	return pd;
}

//Create an antenna with it's gain pattern stored in an XML file
Antenna* rs::createXmlAntenna(const std::string& name, const std::string& file)
{
	auto* fa = new XmlAntenna(name, file);
	return fa;
}

//Create an antenna with it's gain pattern stored in an XML file
Antenna* rs::createFileAntenna(const std::string& name, const std::string& file)
{
	auto* fa = new FileAntenna(name, file);
	return fa;
}

//Create an antenna with gain pattern described by a Python program
Antenna* rs::createPythonAntenna(const std::string& name, const std::string& module, const std::string& function)
{
	auto* pa = new PythonAntenna(name, module, function);
	return pa;
}
