// antenna_factory.h
// Class for Antennas, with different gain patterns, etc.
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 July 2006

#ifndef ANTENNA_FACTORY_H
#define ANTENNA_FACTORY_H

#include <string>

#include "config.h"
#include "geometry_ops.h"
#include "python_extension.h"
#include "boost/utility.hpp"

namespace rs
{
	class SVec3;
	class Vec3;
	class InterpSet;
	class Pattern;

	class Antenna : public boost::noncopyable
	{
	public:
		explicit Antenna(std::string name);

		virtual ~Antenna();

		[[nodiscard]] virtual RS_FLOAT getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const =
		0;

		[[nodiscard]] virtual RS_FLOAT getNoiseTemperature(const SVec3& angle) const;

		void setEfficiencyFactor(RS_FLOAT loss);

		[[nodiscard]] RS_FLOAT getEfficiencyFactor() const;

		[[nodiscard]] std::string getName() const;

	protected:
		static RS_FLOAT getAngle(const SVec3& angle, const SVec3& refangle);

	private:
		RS_FLOAT _loss_factor;
		std::string _name;
	};

	Antenna* createIsotropicAntenna(const std::string& name);

	Antenna* createXmlAntenna(const std::string& name, const std::string& file);

	Antenna* createFileAntenna(const std::string& name, const std::string& file);

	Antenna* createPythonAntenna(const std::string& name, const std::string& module, const std::string& function);

	Antenna* createSincAntenna(const std::string& name, RS_FLOAT alpha, RS_FLOAT beta, RS_FLOAT gamma);

	Antenna* createGaussianAntenna(const std::string& name, RS_FLOAT azscale, RS_FLOAT elscale);

	Antenna* createHornAntenna(const std::string& name, RS_FLOAT dimension);

	Antenna* createParabolicAntenna(const std::string& name, RS_FLOAT diameter);
}

namespace rs_antenna
{
	class Isotropic final : public rs::Antenna
	{
	public:
		explicit Isotropic(const std::string& name);

		~Isotropic() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;
	};

	class Sinc final : public rs::Antenna
	{
	public:
		Sinc(const std::string& name, RS_FLOAT alpha, RS_FLOAT beta, RS_FLOAT gamma);

		~Sinc() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		RS_FLOAT _alpha;
		RS_FLOAT _beta;
		RS_FLOAT _gamma;
	};

	class Gaussian final : public rs::Antenna
	{
	public:
		Gaussian(const std::string& name, RS_FLOAT azscale, RS_FLOAT elscale);

		~Gaussian() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		RS_FLOAT _azscale;
		RS_FLOAT _elscale;
	};

	class SquareHorn final : public rs::Antenna
	{
	public:
		SquareHorn(const std::string& name, RS_FLOAT dimension);

		~SquareHorn() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		RS_FLOAT _dimension;
	};

	class ParabolicReflector final : public rs::Antenna
	{
	public:
		ParabolicReflector(const std::string& name, RS_FLOAT diameter);

		~ParabolicReflector() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		RS_FLOAT _diameter;
	};

	class XmlAntenna final : public rs::Antenna
	{
	public:
		XmlAntenna(const std::string& name, const std::string& filename);

		~XmlAntenna() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		void loadAntennaDescription(const std::string& filename);

		RS_FLOAT _max_gain{};
		rs::InterpSet* _azi_samples;
		rs::InterpSet* _elev_samples;
	};

	class FileAntenna final : public rs::Antenna
	{
	public:
		FileAntenna(const std::string& name, const std::string& filename);

		~FileAntenna() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		rs::Pattern* _pattern;
	};

	class PythonAntenna final : public rs::Antenna
	{
	public:
		PythonAntenna(const std::string& name, const std::string& module, const std::string& function);

		~PythonAntenna() override;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		rs_python::PythonAntennaMod _py_antenna;
	};
}

#endif
