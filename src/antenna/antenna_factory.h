// antenna_factory.h
// Class for Antennas, with different gain patterns, etc.
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 July 2006

#ifndef ANTENNA_FACTORY_H
#define ANTENNA_FACTORY_H

#include <memory>
#include <string>

#include "antenna_pattern.h"
#include "interpolation/interpolation_set.h"
#include "math_utils/geometry_ops.h"
#include "python/python_extension.h"

namespace antenna
{
	class Antenna
	{
	public:
		explicit Antenna(std::string name) : _loss_factor(1), _name(std::move(name)) {}

		virtual ~Antenna() = default;

		// Delete copy constructor and assignment operator to prevent copying
		Antenna(const Antenna&) = delete;

		Antenna& operator=(const Antenna&) = delete;

		[[nodiscard]] virtual RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                                       RealType wavelength) const = 0;

		// TODO: Implement noise temperature calculation
		[[nodiscard]] virtual RealType getNoiseTemperature(const math::SVec3& angle) const { return 0; }

		void setEfficiencyFactor(RealType loss);

		[[nodiscard]] RealType getEfficiencyFactor() const { return _loss_factor; }

		[[nodiscard]] std::string getName() const { return _name; }

	protected:
		static RealType getAngle(const math::SVec3& angle, const math::SVec3& refangle);

	private:
		RealType _loss_factor;
		std::string _name;
	};

	Antenna* createIsotropicAntenna(const std::string& name);

	Antenna* createXmlAntenna(const std::string& name, const std::string& file);

	Antenna* createFileAntenna(const std::string& name, const std::string& file);

	Antenna* createPythonAntenna(const std::string& name, const std::string& module, const std::string& function);

	Antenna* createSincAntenna(const std::string& name, RealType alpha, RealType beta, RealType gamma);

	Antenna* createGaussianAntenna(const std::string& name, RealType azscale, RealType elscale);

	// Note: This function is not used in the codebase
	Antenna* createHornAntenna(const std::string& name, RealType dimension);

	Antenna* createParabolicAntenna(const std::string& name, RealType diameter);

	class Isotropic final : public Antenna
	{
	public:
		explicit Isotropic(const std::string& name) : Antenna(name) {}

		~Isotropic() override = default;

		[[nodiscard]] RealType
		getGain(const math::SVec3& angle, const math::SVec3& refangle, RealType wavelength) const override
		{
			return getEfficiencyFactor();
		}
	};

	class Sinc final : public Antenna
	{
	public:
		Sinc(const std::string& name, const RealType alpha, const RealType beta, const RealType gamma) : Antenna(name),
			_alpha(alpha), _beta(beta), _gamma(gamma) {}

		~Sinc() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		RealType _alpha;
		RealType _beta;
		RealType _gamma;
	};

	class Gaussian final : public Antenna
	{
	public:
		Gaussian(const std::string& name, const RealType azscale, const RealType elscale) : Antenna(name),
			_azscale(azscale),
			_elscale(elscale) {}

		~Gaussian() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		RealType _azscale;
		RealType _elscale;
	};

	class SquareHorn final : public Antenna
	{
	public:
		SquareHorn(const std::string& name, const RealType dimension) : Antenna(name), _dimension(dimension) {}

		~SquareHorn() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		RealType _dimension;
	};

	class ParabolicReflector final : public Antenna
	{
	public:
		ParabolicReflector(const std::string& name, const RealType diameter) : Antenna(name), _diameter(diameter) {}

		~ParabolicReflector() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		RealType _diameter;
	};

	class XmlAntenna final : public Antenna
	{
	public:
		XmlAntenna(const std::string& name, const std::string& filename) : Antenna(name),
		                                                                   _azi_samples(
			                                                                   std::make_unique<interp::InterpSet>()),
		                                                                   _elev_samples(
			                                                                   std::make_unique<interp::InterpSet>())
		{
			loadAntennaDescription(filename);
		}

		~XmlAntenna() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		void loadAntennaDescription(const std::string& filename);

		RealType _max_gain{};
		std::unique_ptr<interp::InterpSet> _azi_samples;
		std::unique_ptr<interp::InterpSet> _elev_samples;
	};

	class FileAntenna final : public Antenna
	{
	public:
		FileAntenna(const std::string& name, const std::string& filename) : Antenna(name),
		                                                                    _pattern(
			                                                                    std::make_unique<Pattern>(filename)) {}

		~FileAntenna() override = default;

		[[nodiscard]] RealType
		getGain(const math::SVec3& angle, const math::SVec3& refangle, RealType wavelength) const override
		{
			return _pattern->getGain(angle - refangle) * getEfficiencyFactor();
		}

	private:
		std::unique_ptr<Pattern> _pattern;
	};

	class PythonAntenna final : public Antenna
	{
	public:
		PythonAntenna(const std::string& name, const std::string& module, const std::string& function) : Antenna(name),
			_py_antenna(module, function) {}

		~PythonAntenna() override = default;

		[[nodiscard]] RealType
		getGain(const math::SVec3& angle, const math::SVec3& refangle, RealType wavelength) const override
		{
			return _py_antenna.getGain(angle - refangle) * getEfficiencyFactor();
		}

	private:
		python::PythonAntennaMod _py_antenna;
	};
}

#endif
