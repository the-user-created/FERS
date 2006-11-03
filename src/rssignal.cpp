//rssignal.cpp - Class to contain an arbitrary frequency domain signal
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//24 May 2006

#include <cmath>
#include "rssignal.h"
#include "rsdebug.h"
#include "fftwcpp/fftwcpp.h"
#include "rsnoise.h"

using namespace rsSignal;
using namespace rs;

/// Doppler shift a signal by the given factor
complex* rsSignal::DopplerShift(complex *td_data, rsFloat factor, unsigned int& size)
{
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Applying doppler shift of %f\n", factor);  
  //Call the SRC algorithm to do the real work
  complex *transtime = SRCDopplerShift(td_data, factor, size);
  //If the doppler code didn't do anything, we need to copy the data to an unaligned array
  if (transtime == td_data) {
    transtime = new complex[size];
    for (unsigned int i = 0; i < size; i++)
      transtime[i] = td_data[i];
  }
  //Return the result
  return transtime;
}
  
/// Shift a signal in frequency by shift Hz
void rsSignal::FrequencyShift(rsSignal::complex* data, rsFloat fs, unsigned int size, rsFloat shift)
{
  //We use a timedomain algorithm for this at the moment, a frequency domain one would be better
  //See equations.tex for details of the algorithm

  //Allocate memory for a time domain version of the signal
  complex *timesig = FFTAlignedMalloc<complex>(size);

  //Zero the right hand side of the frequency data
  for (unsigned int i = size/2; i < size; i++)
    data[i] = 0;
  data[0] /= 2.0;
  //Transform the data into the time domain
  FFTComplex* invplan = FFTManager::Instance()->GetComplexPlanInv(size, true, data, timesig);
  invplan->transform(size, data, timesig);

  //Calculate the shift and phase factors
  rsFloat dsize = static_cast<double>(size);
  shift = shift*(dsize/fs);
  rsFloat theta = (shift-std::floor(shift))*fs/dsize;

  //Loop through the array adding the shift factor
  for (unsigned int i = 0; i < size; i++) {
    rsFloat f = 2*M_PI*(i*shift/dsize-theta);    
    timesig[i] *= complex(std::cos(f), std::sin(f));
    timesig[i] *= 2.0/(dsize); //Normalize the results (FFTW does not normalize by default) and correct for energy loss of hilbert transform
    timesig[i] = complex(timesig[i].real(), 0);
  }
  
  //Transform back into the frequency domain
  FFTComplex *plan = FFTManager::Instance()->GetComplexPlan(size, true, timesig, data);
  plan->transform(size, timesig, data);

  //Clean up
  FFTManager::AlignedFree(timesig);
}

/// Shift a signal in time by shift samples
void rsSignal::TimeShift(rsSignal::complex *data, unsigned int size, rsFloat shift)
{
  //See equations.tex for details of this algorithm
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Shifting by %f samples\n", shift);

  double dsize = static_cast<double>(size);

  //Perform the shift
  for (unsigned int i = 0; i < size/2; i++) {
    rsFloat f = 2*M_PI*(i/dsize)*shift;
    data[i] *= complex(std::cos(f), std::sin(f));
  }
  data[size/2] *= complex(std::cos(M_PI*shift), std::sin(M_PI*shift));

  for (unsigned int i = size/2+1; i < size; i++) {
    rsFloat f = 2*M_PI*(i-dsize)/dsize*shift;
    data[i] *= complex(std::cos(f), std::sin(f));
  }
}

/// Add noise to the signal with the given temperature
void rsSignal::AddNoise(rsSignal::complex *data, rsFloat temperature, unsigned int size, double fs)
{
  //The calculation can be canceled if the noise temperature is zero
  if (temperature == 0)
    return;
  
  rsFloat power = rsNoise::NoiseTemperatureToPower(temperature, fs/2);
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Adding noise of temperature %f power %e voltage %e\n", temperature, power, sqrt(power));
  WGNGenerator generator(1);
  //Generate noise in the frequency domain
  for (unsigned int i = 1; i < size/2; i++) {
    complex ns = sqrt(size)*complex(generator.GetSample()*sqrt(power), generator.GetSample()*sqrt(power));
    data[i] += ns;
    data[size-i] += ns;
    rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Noise %e %e %e %e\n", ns.real(), ns.imag(), data[i].real(), data[i].imag());
  }
}

/// Demodulate a frequency domain signal into time domain I and Q
complex* rsSignal::IQDemodulate(complex *data, unsigned int size, rsFloat scale, rsFloat phase)
{
  //A demonstration that this algorithm is correct can be found in equations.tex

  //Allocate memory for the time domain data
  complex* time_data = FFTAlignedMalloc<complex>(size);
  FFTComplex* invplan = FFTManager::Instance()->GetComplexPlanInv(size, true, data, time_data);

  //Transform the signal into the time domain
  invplan->transform(size, data, time_data);

  // Temporary scan for the largest imag() value, to validate the method
  rsFloat max_imag = 0;
  for (unsigned int i = 0; i < size; i++)
    if (std::fabs(time_data[i].imag()) > max_imag)
      max_imag = std::fabs(time_data[i].imag());
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Max imaginary value: %e (should be zero)\n", max_imag);

  //Following the transform, data should be (within numerical accuracy) purely real  
  for (unsigned int i = 0; i < size; i++)
    time_data[i] = complex(scale*time_data[i].real()*std::cos(phase), scale*time_data[i].real()*std::sin(phase));

  //Return the result
  return time_data;
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
  FFTManager::AlignedFree(data);
}

//Clear the data array, emptying the signal and freeing memory
void Signal::Clear()
{
  FFTManager::AlignedFree(data);
  size = 0;
  rate = 0;
  data = 0; //Set data to zero to prevent multiple frees
}

//Load data into the signal, with the given sample rate and size
void Signal::Load(rsFloat *indata, unsigned int samples, rsFloat samplerate)
{
  size = samples;
  rate = samplerate;
  //Allocate memory for the signal
  data = FFTAlignedMalloc<complex>(samples);
  complex* temp = FFTAlignedMalloc<complex>(samples);
  

  //Perform the FFT on the signal
  FFTComplex* plan = FFTManager::Instance()->GetComplexPlan(samples, true, temp, data);

  //Expand the real data into complex data
  for (unsigned int i = 0; i < samples; i++)
    temp[i] = complex(indata[i], 0);

  //Transform the data into the frequency domain
  plan->transform(samples, temp, data);

  FFTAlignedFree(temp);
}

/// Load data into the signal (frequency domain, complex)
void Signal::Load(rsSignal::complex *indata, unsigned int samples, rsFloat samplerate)
{
  size = samples;
  rate = samplerate;
  //Allocate memory for the signal
  data = reinterpret_cast<complex*>(FFTManager::AlignedMalloc(samples*sizeof(complex)));
  //Copy the data
  std::memcpy(data, indata, samples*sizeof(complex));  
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

/// Get a copy of the frequency domain data
complex* Signal::CopyData() const
{
  complex* result = FFTAlignedMalloc<complex>(size);
  //Copy the data into result
  std::memcpy(result, data, sizeof(complex)*size);
  return result;
}

/// Get the number of samples of padding at the beginning of the pulse
unsigned int Signal::Pad() const
{
  return pad;
}

/// Set the signal to point at new data
void Signal::Reset(rsSignal::complex *newdata, unsigned int newsize, rsFloat newrate)
{
  Clear();
  data = newdata;
  size = newsize;
  rate = newrate;
}
