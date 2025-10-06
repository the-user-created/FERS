// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file target.h
* @brief Defines classes for radar targets and their Radar Cross-Section (RCS) models.
*/

#pragma once

#include <memory>
#include <random>
#include <string>
#include <utility>

#include <libfers/config.h>
#include "object.h"
#include "interpolation/interpolation_set.h"
#include "noise/noise_generators.h"

namespace math
{
	class SVec3;
}

namespace radar
{
	class Platform;

	/**
	* @class RcsModel
	* @brief Base class for RCS fluctuation models.
	*/
	class RcsModel
	{
	public:
		virtual ~RcsModel() = default;

		RcsModel() = default;

		RcsModel(const RcsModel&) = delete;

		RcsModel& operator=(const RcsModel&) = delete;

		RcsModel(RcsModel&&) = delete;

		RcsModel& operator=(RcsModel&&) = delete;

		/**
		* @brief Samples the RCS model to produce a value.
		*
		* @return The sampled RCS value.
		*/
		virtual RealType sampleModel() = 0;
	};

	/**
	* @class RcsConst
	* @brief Constant RCS model.
	*/
	class RcsConst final : public RcsModel
	{
	public:
		/**
		* @brief Samples the constant RCS model.
		*
		* @return The RCS value (always 1.0).
		*/
		RealType sampleModel() override { return 1.0; }
	};

	/**
	* @class RcsChiSquare
	* @brief Chi-square distributed RCS model.
	*/
	class RcsChiSquare final : public RcsModel
	{
	public:
		/**
		* @brief Constructs an RcsChiSquare model.
		*
		* @param rngEngine The random number engine to use.
		* @param k The degrees of freedom for the chi-square distribution.
		*/
		explicit RcsChiSquare(std::mt19937& rngEngine, RealType k) :
			_gen(std::make_unique<noise::GammaGenerator>(rngEngine, k)) {}

		/**
		* @brief Samples the chi-square RCS model.
		*
		* @return The sampled RCS value.
		*/
		RealType sampleModel() override { return _gen->getSample(); }

	private:
		std::unique_ptr<noise::GammaGenerator> _gen; ///< The gamma generator for sampling the chi-square distribution.
	};

	/**
	* @class Target
	* @brief Base class for radar targets.
	*/
	class Target : public Object
	{
	public:
		/**
		* @brief Constructs a radar target.
		*
		* @param platform Pointer to the platform associated with the target.
		* @param name The name of the target.
		* @param seed The seed for the target's internal random number generator.
		*/
		Target(Platform* platform, std::string name, const unsigned seed) :
			Object(platform, std::move(name)), _rng(seed) {}

		/**
		* @brief Gets the RCS value for the target.
		*
		* @param inAngle The incoming angle of the radar wave.
		* @param outAngle The outgoing angle of the reflected radar wave.
		* @param time The current simulation time (default is 0.0).
		* @return The RCS value.
		*/
		virtual RealType getRcs(math::SVec3& inAngle, math::SVec3& outAngle, RealType time) const = 0;

		// [[nodiscard]] virtual math::PsMatrix getPolarization() const { return _psm; }

		// virtual void setPolarization(const math::PsMatrix& in) { _psm = in; }

		/**
		* @brief Gets the target's internal random number generator engine.
		* @return A mutable reference to the RNG engine.
		*/
		[[nodiscard]] std::mt19937& getRngEngine() noexcept { return _rng; }

		/**
		* @brief Sets the RCS fluctuation model.
		*
		* Assigns a new RCS fluctuation model to the target.
		* @param in Unique pointer to the new RCS fluctuation model.
		*/
		void setFluctuationModel(std::unique_ptr<RcsModel> in) { _model = std::move(in); }

	protected:
		// math::PsMatrix _psm; ///< The polarization matrix for the target.
		std::unique_ptr<RcsModel> _model{nullptr}; ///< The RCS fluctuation model for the target.
		std::mt19937 _rng; ///< Per-object random number generator for statistical independence.
	};

	/**
	* @class IsoTarget
	* @brief Isotropic radar target.
	*
	*/
	class IsoTarget final : public Target
	{
	public:
		/**
		* @brief Constructs an isotropic radar target.
		*
		* @param platform Pointer to the platform associated with the target.
		* @param name The name of the target.
		* @param rcs The constant RCS value for the target.
		* @param seed The seed for the target's internal random number generator.
		*/
		IsoTarget(Platform* platform, std::string name, const RealType rcs, const unsigned seed) :
			Target(platform, std::move(name), seed),
			_rcs(rcs) {}

		/**
		* @brief Gets the constant RCS value.
		*
		* @return The constant RCS value, possibly modified by the fluctuation model.
		*/
		RealType getRcs(math::SVec3& /*inAngle*/, math::SVec3& /*outAngle*/, RealType /*time*/) const noexcept override;

		/**
		 * @brief Gets the constant RCS value (without fluctuation model applied).
		 *
		 * @return The constant RCS value.
		 */
		[[nodiscard]] RealType getConstRcs() const noexcept { return _rcs; }

	private:
		RealType _rcs; ///< The constant RCS value for the target.
	};

	/**
	* @class FileTarget
	* @brief File-based radar target.
	*/
	class FileTarget final : public Target
	{
	public:
		/**
		* @brief Constructs a file-based radar target.
		*
		* @param platform Pointer to the platform associated with the target.
		* @param name The name of the target.
		* @param filename The name of the file containing RCS data.
		* @param seed The seed for the target's internal random number generator.
		* @throws std::runtime_error If the file cannot be loaded or parsed.
		*/
		FileTarget(Platform* platform, std::string name, const std::string& filename, unsigned seed);

		/**
		* @brief Gets the RCS value from file-based data for a specific bistatic geometry and time.
		* @param inAngle The incoming angle of the radar wave in the global frame.
		* @param outAngle The outgoing angle of the reflected radar wave in the global frame.
		* @param time The simulation time at which the interaction occurs.
		* @return The Radar Cross Section (RCS) value in meters squared (m²).
		* @throws std::runtime_error If RCS data cannot be retrieved.
		*
		* This function calculates the target's aspect-dependent RCS. The key steps are:
		* 1.  Calculate the bistatic angle bisector in the global simulation coordinate system.
		* 2.  Retrieve the target's own orientation (rotation) at the specified 'time'.
		* 3.  Transform the global bistatic angle into the target's local, body-fixed frame by subtracting
		*     the target's rotation. This is critical, as RCS patterns are defined relative to the target itself.
		* 4.  Use this local aspect angle to look up the azimuthal and elevation RCS values from the loaded data.
		*
		* NOTE: This function returns the raw RCS value (σ), which is linearly proportional to scattered power.
		* The calling physics engine is responsible for converting this to a signal amplitude by taking the
		* square root.
		*/
		RealType getRcs(math::SVec3& inAngle, math::SVec3& outAngle, RealType time) const override;

	private:
		std::unique_ptr<interp::InterpSet> _azi_samples; ///< The azimuthal RCS samples.
		std::unique_ptr<interp::InterpSet> _elev_samples; ///< The elevation RCS samples.
	};

	/**
	* @brief Creates an isotropic target.
	*
	* @param platform Pointer to the platform associated with the target.
	* @param name The name of the target.
	* @param rcs The constant RCS value for the target.
	* @param seed The seed for the target's internal random number generator.
	* @return A unique pointer to the newly created IsoTarget.
	*/
	inline std::unique_ptr<Target> createIsoTarget(Platform* platform, std::string name, RealType rcs, unsigned seed)
	{
		return std::make_unique<IsoTarget>(platform, std::move(name), rcs, seed);
	}

	/**
	* @brief Creates a file-based target.
	*
	* @param platform Pointer to the platform associated with the target.
	* @param name The name of the target.
	* @param filename The name of the file containing RCS data.
	* @param seed The seed for the target's internal random number generator.
	* @return A unique pointer to the newly created FileTarget.
	*/
	inline std::unique_ptr<Target> createFileTarget(Platform* platform, std::string name, const std::string& filename,
	                                                unsigned seed)
	{
		return std::make_unique<FileTarget>(platform, std::move(name), filename, seed);
	}
}
