// target.h
// Defines the class for targets
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#ifndef TARGET_H
#define TARGET_H

#include "config.h"
#include "core/object.h"
#include "math_utils/polarization_matrix.h"

namespace rs
{
	class InterpSet;
	class GammaGenerator;

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

		RS_FLOAT sampleModel() override
		{
			return 1.0;
		}
	};

	class RcsChiSquare final : public RcsModel
	{
	public:
		explicit RcsChiSquare(RS_FLOAT k);

		~RcsChiSquare() override;

		RS_FLOAT sampleModel() override;

	private:
		GammaGenerator* _gen;
	};

	class Target : public Object
	{
	public:
		Target(const Platform* platform, const std::string& name) : Object(platform, name)
		{
			_model = nullptr;
		}

		~Target() override
		{
			delete _model;
		}

		virtual RS_FLOAT getRcs(SVec3& inAngle, SVec3& outAngle) const = 0;

		[[nodiscard]] virtual PsMatrix getPolarization() const
		{
			return _psm;
		}

		virtual void setPolarization(const PsMatrix& in)
		{
			_psm = in;
		}

		virtual void setFluctuationModel(RcsModel* in)
		{
			_model = in;
		}

	protected:
		PsMatrix _psm;
		RcsModel* _model;
	};

	class IsoTarget final : public Target
	{
	public:
		IsoTarget(const Platform* platform, const std::string& name, const RS_FLOAT rcs) :
			Target(platform, name), _rcs(rcs)
		{
		}

		~IsoTarget() override = default;

		RS_FLOAT getRcs(SVec3& inAngle, SVec3& outAngle) const override
		{
			return _model ? _rcs * _model->sampleModel() : _rcs;
		}

	private:
		RS_FLOAT _rcs;
	};

	class FileTarget final : public Target
	{
	public:
		FileTarget(const Platform* platform, const std::string& name, const std::string& filename);

		~FileTarget() override;

		RS_FLOAT getRcs(SVec3& inAngle, SVec3& outAngle) const override;

	private:
		InterpSet* _azi_samples;
		InterpSet* _elev_samples;

		void loadRcsDescription(const std::string& filename) const;
	};

	inline Target* createIsoTarget(const Platform* platform, const std::string& name, const RS_FLOAT rcs)
	{
		return new IsoTarget(platform, name, rcs);
	}

	inline Target* createFileTarget(const Platform* platform, const std::string& name, const std::string& filename)
	{
		return new FileTarget(platform, name, filename);
	}
}

#endif
