//rspath.h
//Structures and Classes for Paths and Coordinates
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 21 April 2006
#ifndef __RSPATH_H
#define __RSPATH_H

#include <config.h>
#include <vector>
#include <stdexcept>
#include "rsgeometry.h"

namespace rs {

/// Coord holds a three dimensional spatial co-ordinate and a time value
struct Coord {
  Vec3 pos; //!< Position in space
  rsFloat t; //!< Time

  /// Less than comparison operator. Operates only on time value.
  bool operator< (const Coord b) const {
    return t < b.t;
  }

  /// Assignment operator
  Coord &operator= (rsFloat a) {
    t = a;
    pos.x = pos.y = pos.z = a;
    return *this;
  }
};

//Support arithmetic on path coords.
/// Componentwise multiply
Coord operator* (Coord a, Coord b);
/// Componentwise add
Coord operator+ (Coord a, Coord b);
/// Componentwise subtract
Coord operator- (Coord a, Coord b);
/// Componentwise divide
Coord operator/ (Coord a, Coord b);
/// Add a scalar to each element
Coord operator+ (Coord a, rsFloat b);
/// Componentwise multiplication by scalar
Coord operator* (Coord a, rsFloat b);
/// Componentwise division by scalar
Coord operator/ (rsFloat a, Coord b);

/// Azimuth-Elevation rotation position and time
//Note that the plane for azimuth is the x-y plane (see Coord)
struct RotationCoord {
  rsFloat azimuth; //!< Angle in X-Y plane (radians)
  rsFloat elevation; //!< Elevation above X-Y plane (radians)
  rsFloat t; //!< Time 
  
  /// Less than comparison operator. Operates only on time value.
  bool operator< (const RotationCoord b) const {
    return t < b.t;
  }

  /// Assignment operator
  RotationCoord &operator= (rsFloat a) {
    azimuth = elevation = t = a;
    return *this;
  }

  /// Constructor. Assign scalar to all elements.
  RotationCoord(rsFloat a = 0):
  azimuth(a), elevation(a), t(a)
  {  
  }
};

/// Componentwise Multiply
RotationCoord operator* (RotationCoord a, RotationCoord b);
/// Componentwise Add
RotationCoord operator+ (RotationCoord a, RotationCoord b);
/// Componentwise Subtract
RotationCoord operator- (RotationCoord a, RotationCoord b);
/// Componentwise Divide
RotationCoord operator/ (RotationCoord a, RotationCoord b);
/// Add a scalare to each component
RotationCoord operator+ (RotationCoord a, rsFloat b);
/// Multiply each component by a scalar
RotationCoord operator* (RotationCoord a, rsFloat b);
/// Divide each component by a scalar
RotationCoord operator/ (rsFloat a, RotationCoord b);

/// Defines the movement of an object through space, governed by time
class Path {
public:
  enum InterpType { RS_INTERP_STATIC, RS_INTERP_LINEAR, RS_INTERP_CUBIC };
  /// Default constructor
  Path(InterpType type = RS_INTERP_STATIC);
  /// Add a co-ordinate to the path
  void AddCoord(Coord &coord);
  /// Finalize the path, so it can be requested with getPosition
  void Finalize();
  /// Get the position an object will be at at a specified time
  Vec3 GetPosition(rsFloat t) const;
  /// Set the interpolation type of the path
  void SetInterp(InterpType settype);
protected:
  std::vector<Coord> coords; //!< Vector of time and space coordinates
  std::vector<Coord> dd; //!< Vector of second derivatives at points (used for cubic interpolation)
  bool final; //!< Is the path finalised?
  InterpType type; //!< Type of interpolation
};

//Function which compares two paths and returns a vector between them
SVec3 Compare(const Path &start, const Path &dest);

/// Defines the rotation of an object in space, governed by time
class RotationPath {
public:
  enum InterpType { RS_INTERP_STATIC, RS_INTERP_CONSTANT, RS_INTERP_LINEAR, RS_INTERP_CUBIC };
  /// Default constructor
  RotationPath(InterpType type = RS_INTERP_STATIC);
  /// Method to add a co-ordinate to the path
  void AddCoord(RotationCoord &coord);
  /// Finalize the path, so it can be requested with getPosition
  void Finalize();
  /// Get the position an object will be at at a specified time
  SVec3 GetPosition(rsFloat t) const;
  /// Set the interpolation type
  void SetInterp(InterpType setinterp);
  /// Set properties for fixed rate motion
  void SetConstantRate(RotationCoord &setstart, RotationCoord &setrate);
protected:
  std::vector<RotationCoord> coords; //!< Vector of time and space coordinates
  std::vector<RotationCoord> dd; //!< Vector of second derivatives at points (used for cubic interpolation)
  bool final; //!< Is the path finalised?

  RotationCoord start; //!< Start point of constant rotation
  RotationCoord rate; //!< Rotation rate of constant rotation (rads per sec)

  InterpType type; //!< Type of interpolation
};

/// Exception class for the path code
class PathException: public std::runtime_error
{
 public:
  PathException(std::string description):
    std::runtime_error("Error While Executing Path Code: "+description)
  {
  }     
};

}

#endif
