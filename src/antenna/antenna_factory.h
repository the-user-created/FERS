/**
 * @file antenna_factory.h
 * @brief Header file defining various types of antennas and their gain patterns.
 *
 * This file provides an abstract base class for different antenna types, along with derived classes representing
 * specific antenna models such as isotropic, sinc, Gaussian, square horn, parabolic, and custom antennas loaded
 * from files or Python modules.
 * Each antenna class provides a method to compute the gain based on the input angles and wavelength.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-07-20
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "antenna_pattern.h"
#include "config.h"
#include "core/logging.h"
#include "interpolation/interpolation_set.h"
#include "math/geometry_ops.h"
#include "python/python_extension.h"

namespace antenna
{
	/**
	 * @class Antenna
	 * @brief Abstract base class representing an antenna.
	 *
	 * This class serves as the base for different types of antennas.
	 * Each derived antenna class must implement the `getGain`
	 * function to compute the gain based on the angles and wavelength.
	 * The base class provides a mechanism to set
	 * and get the efficiency factor of the antenna and retrieve the antenna name.
	 */
	class Antenna
	{
	public:
		/**
		 * @brief Constructs an Antenna object with the given name.
		 *
		 * @param name The name of the antenna.
		 */
		explicit Antenna(std::string name) noexcept : _loss_factor(1), _name(std::move(name)) {}

		/// Virtual destructor.
		virtual ~Antenna() = default;

		/// Deleted copy constructor.
		Antenna(const Antenna&) = delete;

		/// Deleted assignment operator.
		Antenna& operator=(const Antenna&) = delete;

		/**
		 * @brief Computes the gain of the antenna based on the input angle and reference angle.
		 *
		 * @param angle The angle at which the gain is to be computed.
		 * @param refangle The reference angle.
		 * @param wavelength The wavelength of the signal.
		 * @return The gain of the antenna at the specified angle and wavelength.
		 */
		[[nodiscard]] virtual RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                                       RealType wavelength) const = 0;

		/**
		 * @brief Retrieves the efficiency factor of the antenna.
		 *
		 * @return The efficiency factor of the antenna.
		 */
		[[nodiscard]] RealType getEfficiencyFactor() const noexcept { return _loss_factor; }

		/**
		 * @brief Retrieves the name of the antenna.
		 *
		 * @return The name of the antenna.
		 */
		[[nodiscard]] std::string getName() const noexcept { return _name; }
		/**
		 * @brief Computes the noise temperature of the antenna based on the angle.
		 *
		 * @param angle The angle at which the noise temperature is to be computed.
		 * @return The noise temperature of the antenna.
		 */
		// TODO: Implement noise temperature calculation
		[[nodiscard]] virtual RealType getNoiseTemperature(const math::SVec3& /*angle*/) const noexcept { return 0; }

		/**
		 * @brief Sets the efficiency factor of the antenna.
		 *
		 * @param loss The new efficiency factor.
		 */
		void setEfficiencyFactor(RealType loss) noexcept;

	protected:
		/**
		 * @brief Computes the angle between the input and reference angles.
		 *
		 * @param angle The input angle.
		 * @param refangle The reference angle.
		 * @return The computed angle.
		 */
		static RealType getAngle(const math::SVec3& angle, const math::SVec3& refangle) noexcept;

	private:
		RealType _loss_factor; ///< Efficiency factor of the antenna.
		std::string _name; ///< Name of the antenna.
	};

	/**
	 * @class Isotropic
	 * @brief Represents an isotropic antenna with uniform gain in all directions.
	 *
	 * This class models an ideal isotropic antenna, which has a directivity of 1 (0 dB).
	 */
	class Isotropic final : public Antenna
	{
	public:
		/**
		 * @brief Constructs an Isotropic antenna with the given name.
		 *
		 * @param name The name of the antenna.
		 */
		explicit Isotropic(const std::string_view name) : Antenna(name.data()) {}

		/// Destructor.
		~Isotropic() override = default;

		/**
		 * @brief Computes the gain of the isotropic antenna.
		 *
		 * @return The gain of the antenna, which is equal to the efficiency factor.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& /*angle*/, const math::SVec3& /*refangle*/,
		                               RealType /*wavelength*/) const override
		{
			// David Young: Isotropic antennas have a directivity of 1 (or 0 dB),
			// therefore, the gain of the antenna is the efficiency factor
			return getEfficiencyFactor();
		}
	};

	/**
	 * @class Sinc
	 * @brief Represents a sinc function-based antenna gain pattern.
	 *
	 * This antenna has a gain pattern defined by a sinc function, with customizable parameters.
	 */
	class Sinc final : public Antenna
	{
	public:
		/**
		 * @brief Constructs a Sinc antenna with the given parameters.
		 *
		 * @param name The name of the antenna.
		 * @param alpha The alpha parameter.
		 * @param beta The beta parameter.
		 * @param gamma The gamma parameter.
		 */
		Sinc(const std::string_view name, const RealType alpha, const RealType beta, const RealType gamma) :
			Antenna(name.data()), _alpha(alpha), _beta(beta), _gamma(gamma) {}

		/// Destructor.
		~Sinc() override = default;

		/**
		 * @brief Computes the gain of the sinc antenna based on the input parameters.
		 *
		 * @param angle The angle at which the gain is to be computed.
		 * @param refangle The reference angle.
		 * @param wavelength The wavelength of the signal.
		 * @return The computed gain of the antenna.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const noexcept override;

	private:
		RealType _alpha; ///< Parameter defining the shape of the gain pattern.
		RealType _beta; ///< Parameter defining the shape of the gain pattern.
		RealType _gamma; ///< Parameter defining the shape of the gain pattern.
	};

	/**
	 * @class Gaussian
	 * @brief Represents a Gaussian-shaped antenna gain pattern.
	 *
	 * This antenna has a gain pattern that follows a Gaussian distribution.
	 */
	class Gaussian final : public Antenna
	{
	public:
		/**
		 * @brief Constructs a Gaussian antenna with the given parameters.
		 *
		 * @param name The name of the antenna.
		 * @param azscale The azimuth scale factor.
		 * @param elscale The elevation scale factor.
		 */
		Gaussian(const std::string_view name, const RealType azscale, const RealType elscale) : Antenna(name.data()),
			_azscale(azscale), _elscale(elscale) {}

		/// Destructor.
		~Gaussian() override = default;

		/**
		 * @brief Computes the gain of the Gaussian antenna.
		 *
		 * @param angle The angle at which the gain is to be computed.
		 * @param refangle The reference angle.
		 * @param wavelength The wavelength of the signal.
		 * @return The computed gain of the antenna.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const noexcept override;

	private:
		RealType _azscale; ///< Azimuth scale factor.
		RealType _elscale; ///< Elevation scale factor.
	};

	/**
	 * @class SquareHorn
	 * @brief Represents a square horn antenna.
	 *
	 * This antenna models a square horn with a specific dimension.
	 */
	class SquareHorn final : public Antenna
	{
	public:
		/**
		 * @brief Constructs a SquareHorn antenna with the given dimension.
		 *
		 * @param name The name of the antenna.
		 * @param dimension The dimension of the square horn.
		 */
		SquareHorn(const std::string_view name, const RealType dimension) : Antenna(name.data()),
		                                                                    _dimension(dimension) {}

		/// Destructor.
		~SquareHorn() override = default;

		/**
		 * @brief Computes the gain of the square horn antenna.
		 *
		 * @param angle The angle at which the gain is to be computed.
		 * @param refangle The reference angle.
		 * @param wavelength The wavelength of the signal.
		 * @return The computed gain of the antenna.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const noexcept override;

	private:
		RealType _dimension; ///< Dimension of the square horn.
	};

	/**
	 * @class Parabolic
	 * @brief Represents a parabolic reflector antenna.
	 *
	 * This antenna models a parabolic reflector with a specific diameter.
	 */
	class Parabolic final : public Antenna
	{
	public:
		/**
		 * @brief Constructs a Parabolic antenna with the given diameter.
		 *
		 * @param name The name of the antenna.
		 * @param diameter The diameter of the parabolic reflector.
		 */
		Parabolic(const std::string_view name, const RealType diameter) : Antenna(name.data()), _diameter(diameter) {}

		/// Destructor.
		~Parabolic() override = default;

		/**
		 * @brief Computes the gain of the parabolic antenna.
		 *
		 * @param angle The angle at which the gain is to be computed.
		 * @param refangle The reference angle.
		 * @param wavelength The wavelength of the signal.
		 * @return The computed gain of the antenna.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const noexcept override;

	private:
		RealType _diameter; ///< Diameter of the parabolic reflector.
	};

	/**
	 * @class XmlAntenna
	 * @brief Represents an antenna whose gain pattern is defined by an XML file.
	 *
	 * This class models an antenna where the gain pattern is read from an XML file.
	 */
	class XmlAntenna final : public Antenna
	{
	public:
		/**
		 * @brief Constructs an XmlAntenna with the specified name and XML configuration file.
		 *
		 * The constructor loads the azimuth and elevation gain patterns from the provided XML file.
		 *
		 * @param name The name of the antenna.
		 * @param filename The path to the XML file containing the antenna's gain pattern data.
		 */
		XmlAntenna(const std::string_view name, const std::string_view filename) : Antenna(name.data()),
			_azi_samples(std::make_unique<interp::InterpSet>()),
			_elev_samples(std::make_unique<interp::InterpSet>()) { loadAntennaDescription(filename); }

		/// Destructor.
		~XmlAntenna() override = default;

		/**
		* @brief Computes the gain of the antenna based on the input angle and reference angle.
		*
		* @param angle The angle at which the gain is to be computed.
		* @param refangle The reference angle.
		* @param wavelength The wavelength of the signal (not used in this antenna type).
		* @return The gain of the antenna at the specified angle.
		* @throws std::runtime_error If gain values cannot be retrieved from the interpolation sets.
		*/
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const override;

	private:
		/**
		 * @brief Loads the antenna gain pattern from the specified XML file.
		 *
		 * @param filename The path to the XML file containing the antenna's gain pattern data.
		 * @throws std::runtime_error If the XML file cannot be loaded or parsed.
		 */
		void loadAntennaDescription(std::string_view filename);

		RealType _max_gain{}; ///< The maximum gain of the antenna.
		std::unique_ptr<interp::InterpSet> _azi_samples; ///< Interpolation set for azimuth gain samples.
		std::unique_ptr<interp::InterpSet> _elev_samples; ///< Interpolation set for elevation gain samples.
	};

	/**
	 * @class FileAntenna
	 * @brief Represents an antenna whose gain pattern is loaded from a file.
	 *
	 * This class models an antenna with a gain pattern defined in an external file. The gain pattern is stored in a
	 * `Pattern` object, which is used to compute the antenna's gain based on the input angle and reference angle.
	 */
	class FileAntenna final : public Antenna
	{
	public:
		/**
		 * @brief Constructs a FileAntenna with the specified name and gain pattern file.
		 *
		 * The constructor initializes the antenna by loading the gain pattern from the provided file.
		 *
		 * @param name The name of the antenna.
		 * @param filename The path to the file containing the antenna's gain pattern.
		 */
		FileAntenna(const std::string_view name, const std::string_view filename) : Antenna(name.data()),
			_pattern(std::make_unique<Pattern>(filename.data())) {}

		/// Destructor.
		~FileAntenna() override = default;

		/**
		 * @brief Computes the gain of the antenna based on the input angle and reference angle.
		 *
		 * @param angle The angle at which the gain is to be computed.
		 * @param refangle The reference angle.
		 * @return The gain of the antenna at the specified angle.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType /*wavelength*/) const override
		{
			return _pattern->getGain(angle - refangle) * getEfficiencyFactor();
		}

	private:
		std::unique_ptr<Pattern> _pattern; ///< Pointer to the gain pattern object.
	};

	/**
	 * @class PythonAntenna
	 * @brief Represents an antenna with its gain pattern implemented in a Python module.
	 *
	 * This class models an antenna whose gain calculation is delegated to a Python function.
	 * The Python function is invoked through a Python module to compute the gain based on the input angles.
	 */
	class PythonAntenna final : public Antenna
	{
	public:
		/**
		 * @brief Constructs a PythonAntenna with the specified name, Python module, and function.
		 *
		 * @param name The name of the antenna.
		 * @param module The name of the Python module that contains the gain calculation function.
		 * @param function The name of the Python function that computes the antenna's gain.
		 */
		PythonAntenna(const std::string_view name, const std::string_view module, const std::string_view function) :
			Antenna(name.data()), _py_antenna(module.data(), function.data()) {}

		/// Destructor.
		~PythonAntenna() override = default;

		/**
		 * @brief Computes the gain of the antenna based on the input angle and reference angle.
		 *
		 * @param angle The angle at which the gain is to be computed.
		 * @param refangle The reference angle.
		 * @return The gain of the antenna at the specified angle.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType /*wavelength*/) const override
		{
			return _py_antenna.getGain(angle - refangle) * getEfficiencyFactor();
		}

	private:
		python::PythonAntennaMod _py_antenna; ///< Python module and function to compute the gain.
	};
}
