//rstarget.h
//Defines the class for targets
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 26 April 2006

#ifndef RS_TARGET_H
#define RS_TARGET_H

#include "config.h"
#include "rsobject.h"
#include "rspath.h"
#include "rspolarize.h"

namespace rs
{
	//Forward declarations
	class InterpSet; //from rsinterp.h
	class GammaGenerator; //from rsnoise.h


	/// General RCS statistical model class
	class RcsModel
	{
	public:
		/// Destructor
		virtual ~RcsModel();

		/// Get an RCS based on the statistical model and the mean RCS
		virtual RS_FLOAT sampleModel() = 0;
	};

	/// RCS Statistical Model class supporting Swerling V
	class RcsConst final : public RcsModel
	{
	public:
		/// Destructor
		~RcsConst() override;

		/// Return a constant RCS
		RS_FLOAT sampleModel() override;
	};

	/// RCS statistical model following Swerling's Chi-square (actually Gamma) model
	// See Swerling, "Radar Probability of Detection for Some Additional Target Cases", IEEE Trans. Aer. Elec. Sys., Vol 33, 1997
	class RcsChiSquare final : public RcsModel
	{
	public:
		/// Constructor
		explicit RcsChiSquare(RS_FLOAT k); //k is the shape parameter for the distribution
		/// Destructor
		~RcsChiSquare() override;

		/// Get an RCS based on the Swerling II model and the mean RCS
		RS_FLOAT sampleModel() override;

	private:
		GammaGenerator* _gen;
	};

	/// Target models a simple point target with a specified RCS pattern
	class Target : public Object
	{
	public:
		/// Constructor
		Target(const Platform* platform, const std::string& name);

		/// Destructor
		~Target() override;

		/// Returns the Radar Cross Section at a particular angle
		virtual RS_FLOAT getRcs(SVec3& inAngle, SVec3& outAngle) const = 0;

		/// Get the target polarization matrix
		[[nodiscard]] virtual PsMatrix getPolarization() const;

		/// Set the target polarization matrix
		virtual void setPolarization(const PsMatrix& in);

		/// Set the target fluctuation model
		virtual void setFluctuationModel(RcsModel* in);

	protected:
		PsMatrix _psm; //!< Polarization scattering matrix for target interaction
		RcsModel* _model; //!< Statistical model of target RCS fluctuations
	};

	/// Target with an isotropic (constant with angle) RCS
	class IsoTarget final : public Target
	{
	public:
		/// Constructor
		IsoTarget(const Platform* platform, const std::string& name, RS_FLOAT rcs);

		/// Destructor
		~IsoTarget() override;

		/// Return the RCS at the given angle
		RS_FLOAT getRcs(SVec3& inAngle, SVec3& outAngle) const override;

	private:
		RS_FLOAT _rcs; //!< Constant RCS
	};

	/// Target with an RCS interpolated from a table of values
	class FileTarget final : public Target
	{
	public:
		/// Constructor
		FileTarget(const Platform* platform, const std::string& name, const std::string& filename);

		/// Destructor
		~FileTarget() override;

		/// Return the RCS at the given angle
		RS_FLOAT getRcs(SVec3& inAngle, SVec3& outAngle) const override;

	private:
		InterpSet* _azi_samples; //!< Samples of RCS in the azimuth plane
		InterpSet* _elev_samples; //!< Samples of RCS in the elevation plane
		///Load data from the RCS description file
		void loadRcsDescription(const std::string& filename) const;
	};

	// Functions for creating objects of various target types

	/// Create an isometric radiator target
	Target* createIsoTarget(const Platform* platform, const std::string& name, RS_FLOAT rcs);

	/// Create a target, loading the RCS pattern from a file
	Target* createFileTarget(const Platform* platform, const std::string& name, const std::string& filename);
}

#endif
