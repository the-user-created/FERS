//rspulsefactory.cpp
//Functions for generating radar pulses from parameters
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//1 June 2006

#include <stdexcept>
#include <fstream>
#include <boost/shared_array.hpp>
#include "rspulsefactory.h"
#include "rsradarwaveform.h"
#include "rssignal.h"
#include "rsparameters.h"
#include "rsdebug.h"
#include <fftwcpp/fftwcpp.h>

using namespace rs;

namespace rsPulseFactory {
  /// Generate a rect pulse of the specified length
  rs::RadarWaveform* GenerateRectPulse(std::string name, rsFloat length, rsFloat power, rsFloat carrierfreq)
  {
    rsRadarWaveform::MonoPulse *wave;
    wave = new rsRadarWaveform::MonoPulse(name, length, power, carrierfreq);
    return wave;
  }

  /// Generate a monochrome CW
  rs::CWWaveform* GenerateMonoCW(std::string name, rsFloat power, rsFloat carrierfreq)
  {
    rsRadarWaveform::CWMonochrome *mono;
    mono = new rsRadarWaveform::CWMonochrome(name, power, carrierfreq);
    return mono;
  }

  /// Load a pulse from a file and generate an anypulse
  rs::RadarWaveform* GenerateAnyPulseFromFile(const std::string& name, const std::string& filename, rsFloat power, rsFloat carrierfreq)
  {
    ///Open the file
    std::ifstream ifile(filename.c_str());
    if (!ifile)
      throw std::runtime_error("Could not open "+filename+" to read pulse waveform");
    /// Read the length and sample rate
    rsFloat rlength, rate;
    ifile >> rlength; //length in samples
    ifile >> rate; //rate
    unsigned int length = static_cast<int>(rlength);
    //Allocate memory for the file contents
    boost::shared_array<rsFloat> data(new rsFloat[length]);
    //Loop through reading the samples in the file
    unsigned int done = 0;
    while (!ifile.eof() && (done < length))
      ifile >> data[done++];
    if (done != length)
      throw std::runtime_error("Could not read pulse waveform from file "+filename);
    //Create the signal object with the data from the file
    Signal *signal = new Signal();
    signal->Load(data.get(), length, rate);
    //Create the pulse
    rsRadarWaveform::AnyPulse *any = new rsRadarWaveform::AnyPulse(name, length/rate, power, carrierfreq, signal);
    return any;
  }

  /// Generate a rect pulse of the specified length
  rs::RadarWaveform* GenerateRectAnyPulse(const std::string& name, rsFloat length, rsFloat power, rsFloat carrierfreq)
  {
    //Generate the pulse in the time domain for now
    


    //We assign the sample frequency so the pulse is sampled 10 times or so
    unsigned int plen;
    rsFloat rate;
    //If the rate is specified, we must obey that
    if (rsParameters::rate() > 0) {
      rate = rsParameters::rate();
      plen = static_cast<unsigned int>(std::floor(length*rate));
      if (plen < 2)
        throw std::logic_error("[ERROR] Insufficient sample rate to express rect pulse");
    }
    else { //Unspecified rate, use our own
      plen = 10;
      rate = 10.0/length;
    }
    unsigned int size = plen;
    rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Creating rect pulse of length %f size %d rate: %f\n", length, plen, rate);
    //Allocate memory for the pulse
    boost::shared_array<rsSignal::complex> data(new rsSignal::complex[size]);
    //Create the pulse
    memset(data.get(), 0, size*sizeof(rsSignal::complex));
    for (unsigned int i = 0; i < plen; i++)
      data[i] = rsSignal::complex(1, 0);
    // Transform it into the frequency domain
    FFTComplex *plan = FFTManager::Instance()->GetComplexPlan(size, true, data.get(), data.get());
    plan->transform(size, data.get(), data.get());
    //Create the signal object
    Signal *signal = new Signal();
    signal->Load(data.get(), size, rate);
    //Create the pulse
    rsRadarWaveform::AnyPulse *any = new rsRadarWaveform::AnyPulse(name, size/rate, power, carrierfreq, signal);
    return any;    
  }


}
