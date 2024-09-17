// antenna_factory.cpp
// Implementation of Antenna Class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 July 2006

#define TIXML_USE_STL

#include "antenna_factory.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tinyxml.h>
#include <utility>

#include "radar_signal.h"
#include "core/logging.h"
#include "core/portable_utils.h"
#include "interpolation/interpolation_set.h"

using namespace rs;
using namespace rs_antenna;

RS_FLOAT getNodeFloat(const TiXmlHandle& node);

namespace
{
	RS_FLOAT sinc(const RS_FLOAT theta) { return std::sin(theta) / (theta + std::numeric_limits<RS_FLOAT>::epsilon()); }

	RS_FLOAT j1C(const RS_FLOAT x) { return x == 0 ? 1 : portable_utils::besselJ1(x) / x; }

	void loadAntennaGainAxis(const InterpSet* set, const TiXmlHandle& axisXml)
	{
		TiXmlHandle tmp = axisXml.ChildElement("gainsample", 0);
		for (int i = 0; tmp.Element() != nullptr; i++)
		{
			const RS_FLOAT angle = getNodeFloat(tmp.ChildElement("angle", 0));
			const RS_FLOAT gain = getNodeFloat(tmp.ChildElement("gain", 0));
			set->insertSample(angle, gain);
			tmp = axisXml.ChildElement("gainsample", i);
		}
	}
}

// =====================================================================================================================
//
// ANTENNA CLASS
//
// =====================================================================================================================


void Antenna::setEfficiencyFactor(const RS_FLOAT loss)
{
	if (loss > 1) { logging::printf(logging::RS_IMPORTANT, "Using greater than unity antenna efficiency.\n"); }
	_loss_factor = loss;
}


RS_FLOAT Antenna::getAngle(const SVec3& angle, const SVec3& refangle)
{
	SVec3 normangle(angle);
	normangle.length = 1;
	return std::acos(dotProduct(Vec3(normangle), Vec3(refangle)));
}

Antenna* rs::createIsotropicAntenna(const std::string& name) { return new Isotropic(name); }

Antenna* rs::createSincAntenna(const std::string& name, const RS_FLOAT alpha, const RS_FLOAT beta, const RS_FLOAT gamma)
{
	return new Sinc(name, alpha, beta, gamma);
}

Antenna* rs::createGaussianAntenna(const std::string& name, const RS_FLOAT azscale, const RS_FLOAT elscale)
{
	return new Gaussian(name, azscale, elscale);
}

Antenna* rs::createHornAntenna(const std::string& name, const RS_FLOAT dimension)
{
	return new SquareHorn(name, dimension);
}

Antenna* rs::createParabolicAntenna(const std::string& name, const RS_FLOAT diameter)
{
	return new ParabolicReflector(name, diameter);
}

Antenna* rs::createXmlAntenna(const std::string& name, const std::string& file) { return new XmlAntenna(name, file); }

Antenna* rs::createFileAntenna(const std::string& name, const std::string& file) { return new FileAntenna(name, file); }

Antenna* rs::createPythonAntenna(const std::string& name, const std::string& module, const std::string& function)
{
	return new PythonAntenna(name, module, function);
}

// =====================================================================================================================
//
// GAUSSIAN CLASS
//
// =====================================================================================================================

RS_FLOAT Gaussian::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	const SVec3 a = angle - refangle;
	return std::exp(-a.azimuth * a.azimuth * _azscale) * std::exp(-a.elevation * a.elevation * _elscale);
}

// =====================================================================================================================
//
// SINC CLASS
//
// =====================================================================================================================

RS_FLOAT Sinc::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	const RS_FLOAT theta = getAngle(angle, refangle);
	const RS_COMPLEX complex_sinc(sinc(_beta * theta), 0.0);
	const RS_COMPLEX complex_gain = _alpha * std::pow(complex_sinc, RS_COMPLEX(_gamma, 0.0)) * getEfficiencyFactor();
	return std::abs(complex_gain);
}

// =====================================================================================================================
//
// SQUAREHORN CLASS
//
// =====================================================================================================================

RS_FLOAT SquareHorn::getGain(const SVec3& angle, const SVec3& refangle, const RS_FLOAT wavelength) const
{
	const RS_FLOAT ge = 4 * M_PI * _dimension * _dimension / (wavelength * wavelength);
	const RS_FLOAT x = M_PI * _dimension * std::sin(getAngle(angle, refangle)) / wavelength;
	return ge * std::pow(sinc(x), 2) * getEfficiencyFactor();
}

// =====================================================================================================================
//
// PARABOLIC REFLECTOR CLASS
//
// =====================================================================================================================

RS_FLOAT ParabolicReflector::getGain(const SVec3& angle, const SVec3& refangle, const RS_FLOAT wavelength) const
{
	const RS_FLOAT ge = std::pow(M_PI * _diameter / wavelength, 2);
	const RS_FLOAT x = M_PI * _diameter * std::sin(getAngle(angle, refangle)) / wavelength;
	return ge * std::pow(2 * j1C(x), 2) * getEfficiencyFactor();
}

// =====================================================================================================================
//
// XML ANTENNA CLASS
//
// =====================================================================================================================

XmlAntenna::XmlAntenna(const std::string& name, const std::string& filename) : Antenna(name)
{
	_azi_samples = new InterpSet();
	_elev_samples = new InterpSet();
	loadAntennaDescription(filename);
}

XmlAntenna::~XmlAntenna()
{
	delete _azi_samples;
	delete _elev_samples;
}

RS_FLOAT XmlAntenna::getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const
{
	const SVec3 t_angle = angle - refangle;
	return _azi_samples->getValueAt(std::fabs(t_angle.azimuth)) * _elev_samples->getValueAt(std::fabs(t_angle.elevation)) *
		_max_gain * getEfficiencyFactor();
}

void XmlAntenna::loadAntennaDescription(const std::string& filename)
{
	TiXmlDocument doc(filename.c_str());
	if (!doc.LoadFile()) { throw std::runtime_error("[ERROR] Could not load antenna description " + filename); }
	const TiXmlHandle root(doc.RootElement());
	loadAntennaGainAxis(_elev_samples, root.ChildElement("elevation", 0));
	loadAntennaGainAxis(_azi_samples, root.ChildElement("azimuth", 0));
	_max_gain = std::max(_azi_samples->getMax(), _elev_samples->getMax());
	_elev_samples->divide(_max_gain);
	_azi_samples->divide(_max_gain);
}
