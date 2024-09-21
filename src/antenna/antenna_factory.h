// antenna_factory.h
// Class for Antennas, with different gain patterns, etc.
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 July 2006

#ifndef ANTENNA_FACTORY_H
#define ANTENNA_FACTORY_H

#include <memory>
#include <string>

#include "antenna_pattern.h"
#include "config.h"
#include "interpolation/interpolation_set.h"
#include "math_utils/geometry_ops.h"
#include "python/python_extension.h"

namespace rs
{
	class SVec3;
	class Vec3;
	class InterpSet;
	class Pattern;

	class Antenna
	{
	public:
		explicit Antenna(std::string name) : _loss_factor(1), _name(std::move(name)) {}

		virtual ~Antenna() = default;

		// Delete copy constructor and assignment operator to prevent copying
		Antenna(const Antenna&) = delete;

		Antenna& operator=(const Antenna&) = delete;

		[[nodiscard]] virtual RS_FLOAT getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const =
		0;

		// TODO: Implement noise temperature calculation
		[[nodiscard]] virtual RS_FLOAT getNoiseTemperature(const SVec3& angle) const { return 0; }

		void setEfficiencyFactor(RS_FLOAT loss);

		[[nodiscard]] RS_FLOAT getEfficiencyFactor() const { return _loss_factor; }

		[[nodiscard]] std::string getName() const { return _name; }

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

	// Note: This function is not used in the codebase
	Antenna* createHornAntenna(const std::string& name, RS_FLOAT dimension);

	Antenna* createParabolicAntenna(const std::string& name, RS_FLOAT diameter);
}

namespace rs_antenna
{
	class Isotropic final : public rs::Antenna
	{
	public:
		explicit Isotropic(const std::string& name) : Antenna(name) {}

		~Isotropic() override = default;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override
		{
			return getEfficiencyFactor();
		}
	};

	class Sinc final : public rs::Antenna
	{
	public:
		Sinc(const std::string& name, const RS_FLOAT alpha, const RS_FLOAT beta, const RS_FLOAT gamma) : Antenna(name),
			_alpha(alpha), _beta(beta), _gamma(gamma) {}

		~Sinc() override = default;

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
		Gaussian(const std::string& name, const RS_FLOAT azscale, const RS_FLOAT elscale) : Antenna(name),
			_azscale(azscale),
			_elscale(elscale) {}

		~Gaussian() override = default;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		RS_FLOAT _azscale;
		RS_FLOAT _elscale;
	};

	class SquareHorn final : public rs::Antenna
	{
	public:
		SquareHorn(const std::string& name, const RS_FLOAT dimension) : Antenna(name), _dimension(dimension) {}

		~SquareHorn() override = default;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		RS_FLOAT _dimension;
	};

	class ParabolicReflector final : public rs::Antenna
	{
	public:
		ParabolicReflector(const std::string& name, const RS_FLOAT diameter) : Antenna(name), _diameter(diameter) {}

		~ParabolicReflector() override = default;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		RS_FLOAT _diameter;
	};

	class XmlAntenna final : public rs::Antenna
	{
	public:
		XmlAntenna(const std::string& name, const std::string& filename) : Antenna(name),
		_azi_samples(std::make_unique<rs::InterpSet>()), _elev_samples(std::make_unique<rs::InterpSet>())
		{
			loadAntennaDescription(filename);
		}

		~XmlAntenna() override = default;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override;

	private:
		void loadAntennaDescription(const std::string& filename);

		RS_FLOAT _max_gain{};
		std::unique_ptr<rs::InterpSet> _azi_samples;
		std::unique_ptr<rs::InterpSet> _elev_samples;
	};

	class FileAntenna final : public rs::Antenna
	{
	public:
		FileAntenna(const std::string& name, const std::string& filename) : Antenna(name),
		_pattern(std::make_unique<rs::Pattern>(filename)) {}

		~FileAntenna() override = default;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override
		{
			return _pattern->getGain(angle - refangle) * getEfficiencyFactor();
		}

	private:
		std::unique_ptr<rs::Pattern> _pattern;
	};

	class PythonAntenna final : public rs::Antenna
	{
	public:
		PythonAntenna(const std::string& name, const std::string& module, const std::string& function) : Antenna(name),
			_py_antenna(module, function) {}

		~PythonAntenna() override = default;

		[[nodiscard]] RS_FLOAT
		getGain(const rs::SVec3& angle, const rs::SVec3& refangle, RS_FLOAT wavelength) const override
		{
			return _py_antenna.getGain(angle - refangle) * getEfficiencyFactor();
		}

	private:
		rs_python::PythonAntennaMod _py_antenna;
	};
}

#endif
