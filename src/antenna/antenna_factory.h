// antenna_factory.h
// Class for Antennas, with different gain patterns, etc.
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 July 2006

#ifndef ANTENNA_FACTORY_H
#define ANTENNA_FACTORY_H

#include <memory>                             // for make_unique, unique_ptr
#include <string>                             // for string
#include <string_view>                        // for string_view
#include <utility>                            // for move

#include "antenna_pattern.h"                  // for Pattern
#include "config.h"                           // for RealType
#include "interpolation/interpolation_set.h"  // for InterpSet
#include "math_utils/geometry_ops.h"          // for operator-, SVec3
#include "python/python_extension.h"          // for PythonAntennaMod

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

		[[nodiscard]] RealType getEfficiencyFactor() const { return _loss_factor; }
		[[nodiscard]] std::string getName() const { return _name; }
		// TODO: Implement noise temperature calculation
		[[nodiscard]] virtual RealType getNoiseTemperature(const math::SVec3& angle) const { return 0; }

		void setEfficiencyFactor(RealType loss);

	protected:
		static RealType getAngle(const math::SVec3& angle, const math::SVec3& refangle);

	private:
		RealType _loss_factor;
		std::string _name;
	};

	class Isotropic final : public Antenna
	{
	public:
		explicit Isotropic(const std::string_view name) : Antenna(name.data()) {}

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
		Sinc(const std::string_view name, const RealType alpha, const RealType beta, const RealType gamma) :
			Antenna(name.data()), _alpha(alpha), _beta(beta), _gamma(gamma) {}

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
		Gaussian(const std::string_view name, const RealType azscale, const RealType elscale) : Antenna(name.data()),
			_azscale(azscale), _elscale(elscale) {}

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
		SquareHorn(const std::string_view name, const RealType dimension) : Antenna(name.data()),
		                                                                    _dimension(dimension) {}

		~SquareHorn() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		RealType _dimension;
	};

	class ParabolicReflector final : public Antenna
	{
	public:
		ParabolicReflector(const std::string_view name, const RealType diameter) : Antenna(name.data()),
			_diameter(diameter) {}

		~ParabolicReflector() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		RealType _diameter;
	};

	class XmlAntenna final : public Antenna
	{
	public:
		XmlAntenna(const std::string_view name, const std::string_view filename) : Antenna(name.data()),
			_azi_samples(std::make_unique<interp::InterpSet>()),
			_elev_samples(std::make_unique<interp::InterpSet>()) { loadAntennaDescription(filename); }

		~XmlAntenna() override = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		void loadAntennaDescription(std::string_view filename);

		RealType _max_gain{};
		std::unique_ptr<interp::InterpSet> _azi_samples;
		std::unique_ptr<interp::InterpSet> _elev_samples;
	};

	class FileAntenna final : public Antenna
	{
	public:
		FileAntenna(const std::string_view name, const std::string_view filename) : Antenna(name.data()),
			_pattern(std::make_unique<Pattern>(filename.data())) {}

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
		PythonAntenna(const std::string_view name, const std::string_view module, const std::string_view function) :
			Antenna(name.data()), _py_antenna(module.data(), function.data()) {}

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
