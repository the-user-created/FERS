/**
* @file target.h
* @brief Defines classes for radar targets and their Radar Cross Section (RCS) models.
*
* @authors David Young, Marc Brooker
* @date 2006-04-26
*/

#pragma once

#include <memory>
#include <string>
#include <utility>

#include "config.h"
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
		* @param k The degrees of freedom for the chi-square distribution.
		*/
		explicit RcsChiSquare(RealType k) : _gen(std::make_unique<noise::GammaGenerator>(k)) {}

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
		*/
		Target(Platform* platform, std::string name) : Object(platform, std::move(name)) {}

		/**
		* @brief Gets the RCS value for the target.
		*
		* @param inAngle The incoming angle of the radar wave.
		* @param outAngle The outgoing angle of the reflected radar wave.
		* @return The RCS value.
		*/
		virtual RealType getRcs(math::SVec3& inAngle, math::SVec3& outAngle) const = 0;

		// [[nodiscard]] virtual math::PsMatrix getPolarization() const { return _psm; }

		// virtual void setPolarization(const math::PsMatrix& in) { _psm = in; }

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
		*/
		IsoTarget(Platform* platform, std::string name, const RealType rcs) : Target(platform, std::move(name)),
		                                                                      _rcs(rcs) {}

		/**
		* @brief Gets the constant RCS value.
		*
		* @return The constant RCS value, possibly modified by the fluctuation model.
		*/
		RealType getRcs(math::SVec3& /*inAngle*/, math::SVec3& /*outAngle*/) const noexcept override;

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
		* @throws std::runtime_error If the file cannot be loaded or parsed.
		*/
		FileTarget(Platform* platform, std::string name, const std::string& filename);

		/**
		* @brief Gets the RCS value from file-based data.
		*
		* @param inAngle The incoming angle of the radar wave.
		* @param outAngle The outgoing angle of the reflected radar wave.
		* @return The RCS value based on the angle, possibly modified by the fluctuation model.
		* @throws std::runtime_error If RCS data cannot be retrieved.
		*/
		RealType getRcs(math::SVec3& inAngle, math::SVec3& outAngle) const override;

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
	* @return A unique pointer to the newly created IsoTarget.
	*/
	inline std::unique_ptr<Target> createIsoTarget(Platform* platform, std::string name, RealType rcs)
	{
		return std::make_unique<IsoTarget>(platform, std::move(name), rcs);
	}

	/**
	* @brief Creates a file-based target.
	*
	* @param platform Pointer to the platform associated with the target.
	* @param name The name of the target.
	* @param filename The name of the file containing RCS data.
	* @return A unique pointer to the newly created FileTarget.
	*/
	inline std::unique_ptr<Target> createFileTarget(Platform* platform, std::string name, const std::string& filename)
	{
		return std::make_unique<FileTarget>(platform, std::move(name), filename);
	}
}
