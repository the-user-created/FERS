//rsantenna.h
//Class for Antennas, with different gain patterns, etc
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//20 July 2006

#ifndef RS_ANTENNA_H
#define RS_ANTENNA_H

#include <string>

#include "config.h"
#include "rsgeometry.h"
#include "rspython.h"
#include "boost/utility.hpp"

namespace rs
{
	//Forward declaration of SVec3 and Vec3 (see rspath.h)
	class SVec3;
	class Vec3;
	//Forward declaration of InterpSet (see rsinterp.h)
	class InterpSet;
	//Forward declaration of Pattern (see rspattern.h)
	class Pattern;

	/// The antenna class defines an antenna, which may be used by one or more transmitters
	class Antenna : public boost::noncopyable
	{
		//Antennas are not meant to be copied
	public:
		/// Default constructor
		explicit Antenna(const std::string& name);

		/// Destructor
		virtual ~Antenna();

		/// Returns the current gain at a particular angle
		virtual RS_FLOAT getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const = 0;

		/// Returns the noise temperature at a particular angle
		virtual RS_FLOAT getNoiseTemperature(const SVec3& angle) const;

		/// Set the antenna's loss factor (values > 1 are physically impossible)
		void setEfficiencyFactor(RS_FLOAT loss);

		/// Gets the Loss Factor
		RS_FLOAT getEfficiencyFactor() const;

		/// Return the name of the antenna
		std::string getName() const;

	protected:
		/// Get the angle off boresight
		static RS_FLOAT getAngle(const SVec3& angle, const SVec3& refangle);

	private:
		/// The loss factor for this antenna
		RS_FLOAT _loss_factor; //!< Loss factor
		/// The name of the antenna
		std::string _name;
	};

	// Functions to create Antenna objects

	/// Create an Isotropic Antenna
	Antenna* createIsotropicAntenna(const std::string& name);

	/// Create an antenna with it's gain pattern stored in an XML file
	Antenna* createXmlAntenna(const std::string& name, const std::string& file);

	/// Create an antenna with it's gain pattern stored in an HDF5 file
	Antenna* createFileAntenna(const std::string& name, const std::string& file);

	/// Create an antenna with gain pattern described by a Python program
	Antenna* createPythonAntenna(const std::string& name, const std::string& module, const std::string& function);

	/// Create a Sinc Pattern Antenna
	// see rsantenna.cpp for meaning of alpha and beta
	Antenna* createSincAntenna(const std::string& name, RS_FLOAT alpha, RS_FLOAT beta, RS_FLOAT gamma);

	/// Create a Gaussian Pattern Antenna
	Antenna* createGaussianAntenna(const std::string& name, RS_FLOAT azscale, RS_FLOAT elscale);

	/// Create a Square Horn Antenna
	Antenna* createHornAntenna(const std::string& name, RS_FLOAT dimension);

	/// Create a parabolic reflector dish
	Antenna* createParabolicAntenna(const std::string& name, RS_FLOAT diameter);
}

namespace rs_antenna
{
	//Antenna with an Isotropic radiation pattern
	class Isotropic final : public rs::Antenna
	{
	public:
		/// Default constructor
		explicit Isotropic(const std::string& name);

		/// Default destructor
		virtual ~Isotropic();

		/// Get the gain at an angle
		virtual RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;
	};

	//Antenna with a sinc (sinx/x) radiation pattern
	class Sinc final : public rs::Antenna
	{
	public:
		/// Constructor
		Sinc(const std::string& name, RS_FLOAT alpha, RS_FLOAT beta, RS_FLOAT gamma);

		/// Destructor
		virtual ~Sinc();

		/// Get the gain at an angle
		virtual RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;

	private:
		RS_FLOAT _alpha; //!< First parameter (see equations.tex)
		RS_FLOAT _beta; //!< Second parameter (see equations.tex)
		RS_FLOAT _gamma; //!< Third parameter (see equations.tex)
	};

	//Antenna with a Gaussian radiation pattern
	class Gaussian final : public rs::Antenna
	{
	public:
		/// Constructor
		Gaussian(const std::string& name, RS_FLOAT azscale, RS_FLOAT elscale);

		/// Destructor
		virtual ~Gaussian();

		/// Get the gain at an angle
		virtual RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;

	private:
		RS_FLOAT _azscale; //!< Azimuth scale parameter
		RS_FLOAT _elscale; //!< Elevation scale parameter
	};

	/// Square horn antenna
	class SquareHorn final : public rs::Antenna
	{
	public:
		/// Constructor
		SquareHorn(const std::string& name, RS_FLOAT dimension);

		/// Default destructor
		~SquareHorn();

		/// Get the gain at an angle
		RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;

	private:
		RS_FLOAT _dimension; //!< The linear size of the horn
	};

	/// Parabolic dish antenna
	class ParabolicReflector final : public rs::Antenna
	{
	public:
		/// Constructor
		ParabolicReflector(const std::string& name, RS_FLOAT diameter);

		/// Default destructor
		~ParabolicReflector();

		/// Get the gain at an angle
		RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;

	private:
		RS_FLOAT _diameter;
	};

	/// Antenna with gain pattern loaded from and XML description file
	class XmlAntenna final : public rs::Antenna
	{
	public:
		/// Constructor
		XmlAntenna(const std::string& name, const std::string& filename);

		/// Default destructor
		~XmlAntenna();

		/// Get the gain at an angle
		RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;

	private:
		/// Load data from the antenna description file
		void loadAntennaDescription(const std::string& filename);

		RS_FLOAT _max_gain{}; //!< Maximum Antenna gain
		rs::InterpSet* _azi_samples; //!< Samples in the azimuth direction
		rs::InterpSet* _elev_samples; //!< Samples in the elevation direction
	};

	/// Antenna with gain pattern loaded from an HDF5 2D pattern (as made by antennatool)
	class FileAntenna final : public rs::Antenna
	{
	public:
		/// Constructor
		FileAntenna(const std::string& name, const std::string& filename);

		/// Default destructor
		~FileAntenna();

		/// Get the gain at an angle
		RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;

	private:
		/// The antenna gain pattern
		rs::Pattern* _pattern;
	};

	/// Antenna with gain pattern calculated by a Python module
	class PythonAntenna final : public rs::Antenna
	{
	public:
		/// Constructor
		PythonAntenna(const std::string& name, const std::string& module, const std::string& function);

		/// Default destructor
		~PythonAntenna();

		/// Get the gain at an angle
		RS_FLOAT getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const;

	private:
		rs_python::PythonAntennaMod _py_antenna;
	};
}

#endif
