//fftwcpp.h
//Simple C++ wrapper for fftw3 
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za

//The only functions in FFTW3 that are not thread safe are the planner functions//In this wrapper, those functions are protected with boost::mutex, so this
//wrapper should be thread safe when used with Boost.Threads

#ifndef __FFTWCPP_H
#define __FFTWCPP_H

#include <config.h>
#include <complex>
#include <map>
#include <stdexcept>
#include <string>

typedef std::complex<rsFloat> Complex;

class FFTComplex{
public:
  enum fft_direction {FFT_FORWARD=-1, FFT_BACKWARD=1};
  FFTComplex(int size, Complex *in, Complex *out, fft_direction direction);
  ~FFTComplex();
  void transform(int size, Complex *in, Complex *out);
 private:
  void *plan;
};

//FFTManager uses the Singleton pattern to create a single threadsafe repository for FFTW plans
class FFTManager {
 public:
  static FFTManager *Instance();
  //Functions which create and retreive plans
  FFTComplex* GetComplexPlan(int size, bool create = false, Complex *in = 0, Complex *out = 0);
  FFTComplex* GetComplexPlanInv(int size, bool create = false, Complex *in = 0, Complex *out = 0);
  void DestroyPlan(FFTComplex *plan);
  //Functions which allocate properly aligned data
  static void *AlignedMalloc(int size);
  static void AlignedFree(void *ptr);
  //Clean up the Manager and remove all plans
  void Clean();

 protected:
  //Our constructor, copy constructor and assignment operator are private
  FFTManager();
  FFTManager(const FFTManager &);
  FFTManager & operator=(const FFTManager &);
  //The pointer to the one instance
  static FFTManager *instance;
  //The map which contains all the created plans  
  std::map <int, FFTComplex *> complex_plans;  
  std::map <int, FFTComplex *> complex_inv_plans;
};

///Template for aligned memory allocation
template<class T> T* FFTAlignedMalloc(int count)
{
  return reinterpret_cast<T*>(FFTManager::AlignedMalloc(count*sizeof(T)));
}

///Template for aligned memory deallocation
void FFTAlignedFree(void *ptr);

///Clean up the FFTW code, freeing memory
void FFTCleanUp();

class FFTException: public std::runtime_error {
public:
  FFTException(const std::string& str);
};

#endif
