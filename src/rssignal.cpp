//rssignal.cpp - Class to contain an arbitrary frequency domain signal
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//24 May 2006

#include <cmath>
#include <limits>
#include <stdexcept>
#include "rssignal.h"
#include "rsdebug.h"
#include "rsnoise.h"
#include "rsparameters.h"
#include "rsdsp.h"

using namespace rsSignal;
using namespace rs;

namespace {

  /// Compute the zeroth order modified bessel function of the first kind
  // Use the polynomial approximation from section 9.8 of
  // "Handbook of Mathematical Functions" by Abramowitz and Stegun
  rsFloat BesselI0(rsFloat x) {
    rsFloat I0;
    rsFloat t = (x/3.75);
    if (t < 0.0) {
      throw std::logic_error("Modified Bessel approximation only valid for x > 0");
    }
    else if (t <= 1.0) {
      t *= t;
      I0 = 1.0 + t*(3.5156229+t*(3.0899424+t*(1.2067492+t*(0.2659732+t*(0.0360768+t*0.0045813)))));
      //Error bounded to 1.6e-7
    }
    else { //t > 1;
      I0 = 0.39894228+t*(0.01328592+t*(0.00225319+t*(-0.00157565+t*(0.00916281+t*(-0.02057706+t*(0.02635537+t*(-0.01647633+t*0.00392377)))))));
      I0 *= std::exp(x)/std::sqrt(x);
      //Error bounded to 1.9e-7
    }
    return I0;
  }
  
  class InterpFilter {
  public:    
    /// Compute the sinc function at the specified x
    inline rsFloat Sinc(rsFloat x) const;
    /// Compute the value of the interpolation filter at time x
    inline rsFloat interp_filter_compute(rsFloat x) const;
    /// Compute the value of a Kaiser Window at the given x
    inline rsFloat kaiser_win_compute(rsFloat x) const;
    /// Lookup the value of the interpolation filter at time x
    inline rsFloat interp_filter(rsFloat x) const;
    /// Get a pointer to the class instance
    static InterpFilter* GetInstance() {
      if (!instance)
	instance = new InterpFilter();
      return instance;
    }
  private:
    //// Default constructor
    InterpFilter();
    /// Copy Constructor
    InterpFilter(const InterpFilter& ifilt);
    /// Assignment operator
    InterpFilter& operator=(const InterpFilter& ifilt);
    /// Pointer to a single instance of the class
    static InterpFilter *instance;

    rsFloat alpha; //!< 'alpha' parameter
    rsFloat beta; //!< 'beta' parameter
    rsFloat bessel_beta; //!< I0(beta)

    int table_size; //!< Size of the table used for linear interpolation
    rsFloat *interp_table; //!< Table of values used for linear interpolation
    rsFloat table_index_mult; //!< Filter value to table index factor
  };

  /// Interpfilter class constructor
  InterpFilter::InterpFilter()
  {
    //Size of the table to use for interpolation
    table_size = 30000;
    //Allocate memory for the table
    interp_table = new rsFloat[table_size+1];
    //Alpha is half the filter length
    alpha = std::floor(rsParameters::render_filter_length()/2.0);
    //Beta sets the window shape
    beta = 16;
    bessel_beta = BesselI0(beta);    
    //Fill the linear interpolation table
    for (int i = 0; i < table_size; i++)
      interp_table[i] = interp_filter_compute((i/(rsFloat)(table_size))*alpha*2-alpha);
    interp_table[table_size] = 0; //Final element to simplify offset calcs
    //Calculate the multiplication factor for table index calculation
    table_index_mult = table_size/(2*alpha);    
  }
  
  /// Lookup the value of the interpolation filter at time x
  rsFloat InterpFilter::interp_filter(rsFloat x) const {
    bool lookup = true;
    if (lookup) {
      if (x > alpha)
	return 0.0;
      rsFloat wx = (x+alpha)*table_index_mult;
      rsFloat weight = std::fmod(wx, 1);
      int offset = static_cast<int>(wx);
      rsFloat interp = (interp_table[offset]*(1-weight)+interp_table[offset+1]*(weight));
      //rsFloat calc = interp_filter_compute(x)*Sinc(x);
      //       rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "%f %10.20e %e\n", x, interp, Sinc(x*0.8));
      return interp;
    }
    else {
      rsFloat calc = interp_filter_compute(x);
      return calc;
    }


  }

  /// Compute the sinc function at the specified x
  rsFloat InterpFilter::Sinc(rsFloat x) const
  {
    if (x == 0)
      return 1.0;
    return std::sin(x*M_PI)/(x*M_PI);
  }

  /// Compute the value of a Kaiser Window at the given x    
  rsFloat InterpFilter::kaiser_win_compute(rsFloat x) const {
    if ((x < 0) || (x > (alpha*2)))
      return 0;
    else
      return BesselI0(beta*std::sqrt(1-std::pow((x-alpha)/alpha,2)))/bessel_beta;
  }

  /// Compute the value of the interpolation filter at time x
  rsFloat InterpFilter::interp_filter_compute(rsFloat x) const {
    rsFloat w = kaiser_win_compute(x+alpha);
    rsFloat s = Sinc(x*0.7); //The filter value
    rsFloat filt = w*s;
    //      rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "%g %g\n", t, filt);
    return filt;
  }

  //Create an instance of InterpFilter
  InterpFilter* InterpFilter::instance = 0;

}

/// Simulate the effect of and ADC converter on the signal
void rsSignal::ADCSimulate(complex *data, unsigned int size, int bits, rsFloat fullscale)
{
  //Get the number of levels associated with the number of bits
  rsFloat levels = pow(2, bits-1);
  for (unsigned int i = 0; i < size; i++)
    {
      //Simulate the ADC effect on the I and Q samples
      rsFloat I = std::floor(levels*data[i].real()/fullscale)/levels;
      rsFloat Q = std::floor(levels*data[i].imag()/fullscale)/levels;
      //Clamp I and Q to the range, simulating saturation in the adc
      if (I > 1) I = 1;
      else if (I < -1) I = -1;
      if (Q > 1) Q = 1;
      else if (Q < -1) Q = -1;
      //Overwrite data with the results
      data[i] = complex(I, Q);
    }
}     

//
// Signal Implementation
//

//Default constructor for brute signal
Signal::Signal():
  data(0),
  size(0),
  rate(0)
{
}

//Default destructor for brutesignal
Signal::~Signal()
{
  delete[] data;
}

//Clear the data array, emptying the signal and freeing memory
void Signal::Clear()
{
  delete[] data;
  size = 0;
  rate = 0;
  data = 0; //Set data to zero to prevent multiple frees
}

//Load data into the signal, with the given sample rate and size
void Signal::Load(const rsFloat *indata, unsigned int samples, rsFloat samplerate)
{
  //Remove the previous data
  Clear();
  //Set the size and samples attributes
  size = samples;
  rate = samplerate;  
  //Allocate memory for the signal
  data = new complex[samples];
  //Copy the data
  for (unsigned int i = 0; i < samples; i++)
    data[i] = complex(indata[i], 0.0);
}

/// Load data into the signal (time domain, complex)
void Signal::Load(const complex *indata, unsigned int samples, rsFloat samplerate)
{
  //Remove the previous data
  Clear();
  // Get the oversampling ratio
  unsigned int ratio = rsParameters::oversample_ratio();
  //Allocate memory for the signal
  data = new complex[samples*ratio];
  //Set the size and samples attributes
  size = samples*ratio;
  rate = samplerate*ratio;
  if (ratio == 1) {
    //Copy the data (using a loop for now, not sure memcpy() will always work on complex)
    for (unsigned int i = 0; i < samples; i++)
      data[i] = indata[i];
  }
  else {
    // Upsample the data into the target buffer
    Upsample(indata, samples, data, ratio);
  } 
}

//Return the sample rate of the signal
rsFloat Signal::Rate() const
{
  return rate;
}

//Return the size, in samples, of the signal
unsigned int Signal::Size() const
{
  return size;
}

/// Get a copy of the signal domain data
rsFloat* Signal::CopyData() const
{
  rsFloat* result = new rsFloat[size];
  //Copy the data into result
  std::memcpy(result, data, sizeof(rsFloat)*size);
  return result;
}

/// Get the number of samples of padding at the beginning of the pulse
unsigned int Signal::Pad() const
{
  return pad;
}

/// Render the pulse with the specified doppler, delay and amplitude
boost::shared_array<rsComplex> Signal::Render(const std::vector<InterpPoint> &points, rsFloat trans_power, unsigned int &out_size) const
{
  //Allocate memory for rendering
  rsComplex *out = new rsComplex[size];
  out_size = size;
  //Get the sample interval
  rsFloat timestep = 1.0/rate;
  //Create the rendering window
  const unsigned int filt_length = rsParameters::render_filter_length();
  InterpFilter* interp = InterpFilter::GetInstance();
  //Loop through the interp points, rendering each in time
  std::vector<rs::InterpPoint>::const_iterator iter = points.begin();
  std::vector<rs::InterpPoint>::const_iterator next = iter+1;
  if (next == points.end())
    next = iter;
  //Get the delay of the first point
  rsFloat idelay = std::floor(rate*(*iter).delay);
  //Variable to keep track of last delay number, to save re-calculating the filter for static targets
  rsFloat last_delay = 0;
  //Memory to store the filter in
  rsFloat *filt = new rsFloat[filt_length];
  //Loop over the pulse, performing the rendering
  rsFloat sample_time = (*iter).time;
  for (unsigned int i = 0; i < size; i++, sample_time+=timestep)
    {
      //Check if we should move on to the next set of interp points
      if ((sample_time > (*next).time)) {
	iter=next;
	if ((next+1) != points.end())
	  next++;
      }
      //Get the weightings for the parameters
      rsFloat aw = 1, bw = 0;
      if (iter < next) {
	bw = (sample_time-(*iter).time)/((*next).time-(*iter).time);
	aw = 1 - bw;
      }
      //Now calculate the current sample parameters
      rsFloat amplitude = std::sqrt((*iter).power)*aw+std::sqrt((*next).power)*bw;
      rsFloat fdelay = ((*iter).delay*aw+(*next).delay*bw)*rate-idelay;
      rsFloat phase = std::fmod((*iter).phase*aw+(*next).phase*bw, 2*M_PI);
      //rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Amp: %g Delay: %g Phase: %g\n", amplitude, fdelay, phase);
      //Get the start and end times of interpolation      
      unsigned int start = static_cast<unsigned int>(std::floor(std::max(i-fdelay-filt_length/2.0,0.0)));
      unsigned int end = static_cast<unsigned int>(std::floor(std::max(i-fdelay+filt_length/2.0,0.0)));
      //Clamp start and end into the data range
      start = std::min(size, start);
      end = std::min(size, end);
      if ((end-start) > filt_length) {	
	throw std::logic_error("[BUG] Interpolation length is longer than filter");	
      }
      //Re-calculate the delay filter for the given delay
      if ((fdelay != last_delay) || (i <= (filt_length/2))) { //don't re-calculate the filter if fdelay hasn't changed
	for (unsigned int j = 0; j < (end-start); j++) {
	  filt[j] = interp->interp_filter(i-fdelay-j-start);
	}
      }
      //Apply the filter
      complex accum = 0;      
      for (unsigned int j = start; j < end; j++) {
        accum += amplitude*data[j]*filt[j-start];
      }      
      //Perform IQ demodulation
      out[i] = rs::rsComplex(std::cos(phase)*accum.real()+std::sin(phase)*accum.imag(), -std::sin(phase)*accum.real()+std::cos(phase)*accum.imag());

      //Update the last delay value
      last_delay = fdelay;
    }
  //Clean up the filter memory
  delete[] filt;
  //Return the result
  return boost::shared_array<rs::rsComplex>(out);
}

