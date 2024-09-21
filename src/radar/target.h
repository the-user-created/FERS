// target.h
// Defines the class for targets
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#ifndef TARGET_H
#define TARGET_H

#include <memory>

#include "config.h"
#include "object.h"
#include "interpolation/interpolation_set.h"
#include "math_utils/polarization_matrix.h"
#include "noise/noise_generators.h"

namespace radar
{
	class RcsModel
	{
	public:
		virtual ~RcsModel() = default;

		virtual RS_FLOAT sampleModel() = 0;
	};

	class RcsConst final : public RcsModel
	{
	public:
		~RcsConst() override = default;

		RS_FLOAT sampleModel() override { return 1.0; }
	};

	class RcsChiSquare final : public RcsModel
	{
	public:
		explicit RcsChiSquare(RS_FLOAT k) : _gen(std::make_unique<noise::GammaGenerator>(k)) {}

		~RcsChiSquare() override = default;

		RS_FLOAT sampleModel() override { return _gen->getSample(); }

	private:
		std::unique_ptr<noise::GammaGenerator> _gen;
	};

	class Target : public Object
	{
	public:
		Target(const Platform* platform, const std::string& name) : Object(platform, name), _model(nullptr) {}

		~Target() override = default;

		virtual RS_FLOAT getRcs(math::SVec3& inAngle, math::SVec3& outAngle) const = 0;

		// Note: This function is not used in the codebase
		[[nodiscard]] virtual math::PsMatrix getPolarization() const { return _psm; }

		// Note: This function is not used in the codebase
		virtual void setPolarization(const math::PsMatrix& in) { _psm = in; }

		void setFluctuationModel(std::unique_ptr<RcsModel> in) { _model = std::move(in); }

	protected:
		math::PsMatrix _psm;
		std::unique_ptr<RcsModel> _model;
	};

	class IsoTarget final : public Target
	{
	public:
		IsoTarget(const Platform* platform, const std::string& name, const RS_FLOAT rcs) :
			Target(platform, name), _rcs(rcs) {}

		~IsoTarget() override = default;

		RS_FLOAT getRcs(math::SVec3& inAngle, math::SVec3& outAngle) const override
		{
			return _model ? _rcs * _model->sampleModel() : _rcs;
		}

	private:
		RS_FLOAT _rcs;
	};

	class FileTarget final : public Target
	{
	public:
		FileTarget(const Platform* platform, const std::string& name, const std::string& filename) :
			Target(platform, name), _azi_samples(std::make_unique<interp::InterpSet>()),
			_elev_samples(std::make_unique<interp::InterpSet>()) { loadRcsDescription(filename); }

		~FileTarget() override = default;

		RS_FLOAT getRcs(math::SVec3& inAngle, math::SVec3& outAngle) const override;

	private:
		std::unique_ptr<interp::InterpSet> _azi_samples;
		std::unique_ptr<interp::InterpSet> _elev_samples;

		void loadRcsDescription(const std::string& filename) const;
	};

	inline std::unique_ptr<Target> createIsoTarget(const Platform* platform, const std::string& name,
	                                               const RS_FLOAT rcs)
	{
		return std::make_unique<IsoTarget>(platform, name, rcs);
	}

	inline std::unique_ptr<Target> createFileTarget(const Platform* platform, const std::string& name,
	                                                const std::string& filename)
	{
		return std::make_unique<FileTarget>(platform, name, filename);
	}
}

#endif
