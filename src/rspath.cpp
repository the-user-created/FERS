//rspath.cpp
//Implementation of rotation and position path classes
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//21 April 2006

#include <cmath> //for fmod
#include <algorithm>
#include "rspath.h"
#include "rsdebug.h"

using namespace rs;

//The interpolation functions are implemented as template functions to ease adding more functions to both rotation and motion classes

//Static "interpolation" function - the path is at the same point all the time
template <typename T> void GetPositionStatic(rsFloat t, T &coord, const std::vector<T> &coords) {
  if (coords.empty())
    throw PathException("coord list empty during GetPositionStatic");
  coord = coords[0];
}

//Linear interpolation function
template <typename T> void GetPositionLinear(rsFloat t, T &coord, const std::vector<T> &coords ) {
  T sKey;
  sKey = 0;
  sKey.t = t;
  typename std::vector<T>::const_iterator xrp;
  xrp = upper_bound(coords.begin(), coords.end(), sKey);
  //Check if we are over one of the end points
  if (xrp == coords.begin())
    coord = *xrp; //We are at the left endpoint
  else if (xrp == coords.end())
    coord = *(xrp-1); //We are at the right endpoint
  else { //We are at neither endpoint - perform linear interpolation
    int xri = xrp - coords.begin();
    int xli = xri-1;

    rsFloat iw = coords[xri].t - coords[xli].t;
    rsFloat rw = (coords[xri].t - t)/iw;
    rsFloat lw = 1-rw;
    //Insert the interpolated values in coord
    coord = coords[xri]*lw + coords[xli]*rw;
  }
  //Set the time part of the coordinate
  coord.t = t;
}

//Cubic spline interpolation function 
//The method used (but not the code) is from
//Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T> void GetPositionCubic(rsFloat t, T &coord, const std::vector<T> &coords, const std::vector <T> &dd)
{
  T sKey;
  sKey = 0;
  sKey.t = t;
  typename std::vector<T>::const_iterator xrp;
  //Check that we are finalized, if not, complain
  xrp=upper_bound(coords.begin(), coords.end(), sKey);
  //Check if we are over one of the end points
  if (xrp == coords.begin())
    coord = *xrp; //We are at the left endpoint
  else if (xrp == coords.end())
    coord = *(xrp-1); //We are at the right endpoint
  else { //We are at neither endpoint - perform cubic spline interpolation
    int xri = xrp-coords.begin();
    int xli = xri - 1;
    rsFloat xrd = (coords[xri].t - t), xld = (t - coords[xli].t), iw = (coords[xri].t-coords[xli].t), iws=iw*iw/6.0;
    rsFloat A = xrd/iw, B=xld/iw, C=(A*A*A-A)*iws, D=(B*B*B-B)*iws;
    coord = coords[xli]*A + coords[xri]*B + dd[xli]*C + dd[xri]*D;
  }
  //Set the time part of the coordinate
  coord.t = t;
}

//Finalize function to calculate vector of second derivatives
//The method used (but not the code) is from
//Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T> void finalizeCubic(std::vector <T> &coords, std::vector <T> &dd) {
  int size = coords.size();
  std::vector <T> tmp(size);
  dd.resize(size);

  //Set the second derivative at the end points to zero
  dd[0] = 0;
  dd[size-1] = 0;
  //Forward pass of calculating the second derivatives at each point
  for (int i = 1; i < size-1; i++)
    {
      T yrd = coords[i+1]-coords[i], yld=coords[i]-coords[i-1];
      rsFloat xrd = coords[i+1].t-coords[i].t, xld=coords[i].t-coords[i-1].t;
      rsFloat iw = coords[i+1].t-coords[i-1].t;
      T p = dd[i-1]*(xld/iw)+2.0;
      dd[i] = (xld/iw-1)/p;
      tmp[i] = ((yrd*xrd - yld*xld)*6.0 - tmp[i-1]*xld)/(p*iw);
    }
  //Second (backward) pass of calculation
  for (int i = size-2; i > 0; i--)
    dd[i] = dd[i]*dd[i+1]+tmp[i];
}


//
// Path Implementation
//
Path::Path(Path::InterpType type):
  final(false), type(type)
{
}

void Path::AddCoord(Coord& coord) {
  std::vector < Coord > :: iterator iter;  
  //Find the position to insert the coordinate, preserving sort
  iter = lower_bound(coords.begin(), coords.end(), coord);
  //Insert the new coordinate
  coords.insert(iter, coord);
  //We are not finalized if we have inserted a coord
  final = false;
}

//Get the position of the path object at a specified time
Vec3 Path::GetPosition(rsFloat t) const {
  Coord coord;
  if (!final)
    throw PathException("Finalize not called before GetPosition");
  //Call the interpolation function relevent to the type
  switch (type) {
  case RS_INTERP_STATIC:
    GetPositionStatic<Coord>(t, coord, coords);
    break;
  case RS_INTERP_LINEAR:
    GetPositionLinear<Coord>(t, coord, coords);
    break;
  case RS_INTERP_CUBIC:
    GetPositionCubic<Coord>(t, coord, coords, dd);
    break;
  }
  //Return the position part of the result
  return coord.pos;
}

//Finalize the path - doing some once-per-path calculations if necessary
void Path::Finalize()
{
  if (!final) {  
    switch (type) {
    case RS_INTERP_STATIC:
      break;
    case RS_INTERP_LINEAR:
      break;
    case RS_INTERP_CUBIC:
      finalizeCubic<Coord>(coords, dd);
    }
    final = true;
  }
}

//Set the interpolation type of the path
void Path::SetInterp(InterpType settype)
{
  final = false;
  type = settype;
}

//Compares two paths at the same time and returns a vector with the distance and angle
SVec3 Compare(const rsFloat time, Path &start, Path &end)
{
  Vec3 difference = end.GetPosition(time)-start.GetPosition(time);
  SVec3 result(difference); //Get the result in spherical co-ordinates
  return result;
}


//
// RotationPath Implementation
//
RotationPath::RotationPath(RotationPath::InterpType type):
  final(false), start(0), rate(0), type(type)
{
}

void RotationPath::AddCoord(RotationCoord& coord) {
  std::vector < RotationCoord > :: iterator iter;  
  //Find the position to insert the coordinate, preserving sort
  iter = lower_bound(coords.begin(), coords.end(), coord);
  //Insert the new coordinate
  coords.insert(iter, coord);
  //We are not finalized if we have inserted a coord
  final = false;
}

//Get the position of the path object at a specified time
SVec3 RotationPath::GetPosition(rsFloat t) const {
  RotationCoord coord;
  if (!final)
    throw PathException("Finalize not called before GetPosition in Rotation");
  //Call the interpolation function relevent to the type
  switch (type) {
  case RS_INTERP_STATIC:
    GetPositionStatic<RotationCoord>(t, coord, coords);
    break;
  case RS_INTERP_LINEAR:
    GetPositionLinear<RotationCoord>(t, coord, coords);
    break;
  case RS_INTERP_CUBIC:
    GetPositionCubic<RotationCoord>(t, coord, coords, dd);
    break;
  case RS_INTERP_CONSTANT:
    coord.t = t;
    coord.azimuth = std::fmod(t*rate.azimuth+start.azimuth, static_cast<rsFloat>(2*M_PI));
    coord.elevation = std::fmod(t*rate.elevation+start.elevation, static_cast<rsFloat>(2*M_PI));
    break;
  }
  return SVec3(1, coord.azimuth, coord.elevation);
}

//Finalize the path - doing some once-per-path calculations if necessary
void RotationPath::Finalize()
{
  if (!final) {  
    switch (type) {
    case RS_INTERP_STATIC:
      break;
    case RS_INTERP_LINEAR:
      break;
    case RS_INTERP_CONSTANT:
      break;
    case RS_INTERP_CUBIC:
      finalizeCubic<RotationCoord>(coords, dd);
    }
    final = true;
  }
}

//Set the interpolation type
void RotationPath::SetInterp(InterpType setinterp)
{
  type = setinterp;
  final = false;
}

//Set properties for fixed rate motion
void RotationPath::SetConstantRate(RotationCoord &setstart, RotationCoord &setrate)
{
  start = setstart;
  rate = setrate;
  type = RS_INTERP_CONSTANT;
  final = true;
}

//
// Coord Implementation
//

//Componentwise multiplication of space coordinates
Coord rs::operator* (Coord a, Coord b)
{
  Coord c;
  c.pos = a.pos * b.pos;
  c.t = a.t; //Only multiply space coordinates
  return c;
}

//Componentwise addition of space coordinates
Coord rs::operator+ (Coord a, Coord b)
{
  Coord c;
  c.pos = a.pos;
  c.pos += b.pos;
  c.t = a.t; //Only add space coordinates
  return c;
}

//Componentwise subtraction of space coordinates
Coord rs::operator- (Coord a, Coord b)
{
  Coord c;
  c.pos = a.pos;
  c.pos -= b.pos;
  c.t = a.t;
  return c;
}

//Componentwise division of space coordinates
Coord rs::operator/ (Coord a, Coord b)
{
  Coord c;
  c.pos = a.pos / b.pos;
  c.t = a.t; //Only add space coordinates
  return c;
}

//Add a constant to a PathCoord
Coord rs::operator+ (Coord a, rsFloat b)
{
  Coord c;
  c.pos += b;
  c.t = a.t;
  return c;
}

//Multiply by a rsFloat constant
Coord rs::operator* (Coord a, rsFloat b)
{
  Coord c;
  c.pos = a.pos * b;
  c.t = a.t;
  return c;
}

Coord rs::operator/ (rsFloat a, Coord b)
{
  Coord c;
  c.pos = a / b.pos;
  c.t = b.t;
  return c;
}

//
// RotationCoord Implementation
//

//Componentwise multiplication of space coordinates
RotationCoord rs::operator* (RotationCoord a, RotationCoord b)
{
  RotationCoord c;
  c.azimuth = a.azimuth*b.azimuth;
  c.elevation = a.elevation*b.elevation;
  c.t = a.t; //Onlelevation multiply space coordinates
  return c;
}

//Componentwise addition of space coordinates
RotationCoord rs::operator+ (RotationCoord a, RotationCoord b)
{
  RotationCoord c;
  c.azimuth = a.azimuth+b.azimuth;
  c.elevation = a.elevation+b.elevation;
  c.t = a.t; //Only add space coordinates
  return c;
}

//Componentwise subtraction of space coordinates
RotationCoord rs::operator- (RotationCoord a, RotationCoord b)
{
  RotationCoord c;
  c.azimuth = a.azimuth-b.azimuth;
  c.elevation = a.elevation-b.elevation;
  c.t = a.t;
  return c;
}

//Componentwise division of space coordinates
RotationCoord rs::operator/ (RotationCoord a, RotationCoord b)
{
  RotationCoord c;
  c.azimuth = a.azimuth/b.azimuth;
  c.elevation = a.elevation/b.elevation;
  c.t = a.t; //Only add space coordinates
  return c;
}

//Add a constant to a PathRotationCoord
RotationCoord rs::operator+ (RotationCoord a, rsFloat b)
{
  RotationCoord c;
  c.azimuth = a.azimuth+b;
  c.elevation = a.elevation+b;
  c.t = a.t;
  return c;
}

//Multiply by a rsFloat constant
RotationCoord rs::operator* (RotationCoord a, rsFloat b)
{
  RotationCoord c;
  c.azimuth = a.azimuth*b;
  c.elevation = a.elevation*b;
  c.t = a.t;
  return c;
}

RotationCoord rs::operator/ (rsFloat a, RotationCoord b)
{
  RotationCoord c;
  c.azimuth = a/b.azimuth;
  c.elevation = a/b.elevation;
  c.t = b.t;
  return c;
}
