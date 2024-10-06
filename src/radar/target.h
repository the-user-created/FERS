// target.h
// Defines the class for targets
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <utility>

#include "config.h"
#include "object.h"
#include "interpolation/interpolation_set.h"
#include "math/polarization_matrix.h"
#include "noise/noise_generators.h"

namespace math
{
	class SVec3;
}

namespace radar
{
	class Platform;

	// C++20 Concept for RcsModel to enforce `sampleModel` existence
	template <typename T>
	concept RcsModelConcept = requires(T a)
	{
		{ a.sampleModel() } -> std::convertible_to<RealType>;
	};

	class RcsModel
	{
	public:
		virtual ~RcsModel() = default;

		virtual RealType sampleModel() = 0;
	};

	class RcsConst final : public RcsModel
	{
	public:
		~RcsConst() override = default;

		RealType sampleModel() override { return 1.0; }
	};

	class RcsChiSquare final : public RcsModel
	{
	public:
		explicit RcsChiSquare(RealType k) : _gen(std::make_unique<noise::GammaGenerator>(k)) {}

		~RcsChiSquare() override = default;

		RealType sampleModel() override { return _gen->getSample(); }

	private:
		std::unique_ptr<noise::GammaGenerator> _gen;
	};

	class Target : public Object
	{
	public:
		Target(Platform* platform, std::string name) : Object(platform, std::move(name)) {}

		~Target() override = default;

		virtual RealType getRcs(math::SVec3& inAngle, math::SVec3& outAngle) const = 0;

		// [[nodiscard]] virtual math::PsMatrix getPolarization() const { return _psm; }

		// virtual void setPolarization(const math::PsMatrix& in) { _psm = in; }
		void setFluctuationModel(std::unique_ptr<RcsModel> in) { _model = std::move(in); }

	protected:
		math::PsMatrix _psm;
		std::unique_ptr<RcsModel> _model{nullptr};
	};

	class IsoTarget final : public Target
	{
	public:
		IsoTarget(Platform* platform, std::string name, const RealType rcs) : Target(platform, std::move(name)),
		                                                                      _rcs(rcs) {}

		~IsoTarget() override = default;

		RealType getRcs(math::SVec3& /*inAngle*/, math::SVec3& /*outAngle*/) const override;

	private:
		RealType _rcs;
	};

	class FileTarget final : public Target
	{
	public:
		FileTarget(Platform* platform, std::string name, const std::string& filename);

		~FileTarget() override = default;

		RealType getRcs(math::SVec3& inAngle, math::SVec3& outAngle) const override;

	private:
		std::unique_ptr<interp::InterpSet> _azi_samples;
		std::unique_ptr<interp::InterpSet> _elev_samples;

		void loadRcsDescription(const std::string& filename) const;
	};

	// Factory functions
	inline std::unique_ptr<Target> createIsoTarget(Platform* platform, std::string name, RealType rcs)
	{
		return std::make_unique<IsoTarget>(platform, std::move(name), rcs);
	}

	inline std::unique_ptr<Target> createFileTarget(Platform* platform, std::string name, const std::string& filename)
	{
		return std::make_unique<FileTarget>(platform, std::move(name), filename);
	}
}
