//rsmultipath.h
//Classes and definitions for multipath propagation
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//9 September 2007

#ifndef RS_MULTIPATH_H
#define RS_MULTIPATH_H

#include "rsgeometry.h"

namespace rs
{
	/// Class which manages multipath propagation
	class MultipathSurface
	{
	public:
		// These parameters define a plane of the form a*x+b*y+c*z+d=0
		/// Constructor
		MultipathSurface(rsFloat a, rsFloat b, rsFloat c, rsFloat d, rsFloat factor);

		/// Default destructor
		~MultipathSurface();

		/// Reflect a point in the surface
		Vec3 reflectPoint(const Vec3& b) const;

		/// Get the reflectance factor
		rsFloat getFactor() const;

	private:
		rsFloat _factor; //!< How energy is reflected from the plane
		Matrix3 _reflection; //!< Matrix defining reflection in this plane
		rsFloat _norm_factor; //!< Factor to normalize length
		Vec3 _translation_vector; //!< Vector to translate position of reflected point
	};
}

#endif
