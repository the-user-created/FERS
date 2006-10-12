//rssrcdoppler.cpp - Doppler simulation using SRC algorithms
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//05 October 2006

//A note on this approach to doppler:
//This approach is very simple and non-optimal from a performance perspective.
//The advantage is that it's very easy to show correctness, and that libsamplerate's performance and accuracy are excellent (-97dB THD+N quoted)

#include <samplerate.h>
#include <stdexcept>
#include <fftwcpp/fftwcpp.h>
#include "rssignal.h"


using namespace rsSignal;

///Simulate a doppler shift of the given factor
complex* rsSignal::SRCDopplerShift(complex* data, rsFloat factor, unsigned int& size)
{
  //Allocate memory for the transform
  unsigned int newsize = static_cast<int>(std::ceil(factor*size));
  //If the doppler shift is less than one sample, don't bother
  if (size == newsize)
    return data;
  //Convert the double data into float data
  float* fldata = FFTAlignedMalloc<float>(size*2);
  for (unsigned int i = 0; i < size; i++) {
    fldata[i*2] = data[i].real();
    fldata[i*2+1] = data[i].imag();
  }
  FFTAlignedFree(data);  
  //Allocate memory for the transformed data
  float* newdata = FFTAlignedMalloc<float>(newsize*2);
  //Fill libsamplerate's data structure
  SRC_DATA src;
  src.data_in = fldata;
  src.data_out = newdata;
  src.input_frames = size;
  src.output_frames = newsize;
  src.src_ratio = factor;
  //Call libsamplerate to do the conversion
  int src_return = src_simple(&src, SRC_SINC_BEST_QUALITY, 2);
  if (src_return != 0) {
    std::string error(src_strerror(src_return));
    throw std::runtime_error("[ERROR] Libsamplerate failed during doppler calculation. Message:\n"+error);
  }
  //Transform the new data into complex data
  FFTAlignedFree(fldata);
  complex *c_data = FFTAlignedMalloc<complex>(newsize);
  for (unsigned int i = 0; i < newsize; i++)
    c_data[i] = complex(static_cast<double>(newdata[i*2]), static_cast<double>(newdata[i*2+1]));

  FFTAlignedFree(newdata);
  //Set the new size
  size = newsize;
  return c_data;
}
  
