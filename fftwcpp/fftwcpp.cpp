//Implementation of fft_manager.h and fft_plan.h
//See that file for information on what it does and how
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za

//07/04/2006: Initial Revision

#include <stdexcept>
#include <map>
#include <fftw3.h>
#include "fftwcpp.h"
#include <boost/thread/mutex.hpp> //Included for boost::mutex
#include "fftwcpp_templates.h"


//Mutexes for accessing the planner, local to this module
boost::mutex plannerMutex;

FFTManager* FFTManager::instance = 0;

///Aligned memory deallocation
void FFTAlignedFree(void *ptr)
{
  FFTManager::AlignedFree(ptr);
}

///Clean up the FFTW code, freeing memory
void FFTCleanUp()
{
  FFTManager::Instance()->Clean();
}

//Constructor for FFTManager
//This constructor is private and is only called once
FFTManager::FFTManager() {
}

//Function which returns a pointer to the only instance of FFTManager
FFTManager *FFTManager::Instance() {
  if (instance == 0)
    instance = new FFTManager;
  return instance;
}

void * FFTManager::AlignedMalloc(int size) {
  return fftw_malloc(size);
}

void FFTManager::AlignedFree(void *ptr) {
  fftw_free(ptr);
}

//Get a reference to a complex plan, optionally creating it if it does not exist 
FFTComplex * FFTManager::GetComplexPlan(int size, bool create, Complex *in, Complex *out) {
  FFTComplex *plan;
  plan = complex_plans[size];
  //If the plan does not exist, we can optionally create it
  if ((plan == 0) && (create)) {
    plan = new FFTComplex(size, in, out, FFTComplex::FFT_FORWARD);
    complex_plans[size] = plan;
  }
  //Return the plan, or NULL if there is no plan
  return plan;
}

FFTComplex *FFTManager::GetComplexPlanInv(int size, bool create, Complex *in, Complex *out) {
  FFTComplex *plan;
  plan = complex_inv_plans[size];
  //If the plan does not exist, we can optionally create it
  if ((plan == 0) && (create)) {
    plan = new FFTComplex(size, in, out, FFTComplex::FFT_BACKWARD);
    complex_inv_plans[size] = plan;
  }
  //Return the plan
  if (plan)
    return plan;
  else
    throw FFTException("Could not create complex plan");
}

//Clean up the manager and destroy all plans
void FFTManager::Clean() {
  std::map <int, FFTComplex *>::iterator iter;
  for (iter = complex_plans.begin(); iter != complex_plans.end(); iter++)
    delete (*iter).second;
  complex_plans.clear();
  for (iter = complex_inv_plans.begin(); iter != complex_inv_plans.end(); iter++)
    delete (*iter).second;
  complex_inv_plans.clear();
}

//
// Implementation of FFTComplex
//

//Perform an FFT on two arrays of data
void FFTComplex::transform(int size, Complex *in, Complex *out)
{
  if (plan == 0)
    throw FFTException("Can not perform transform on NULL plan.");
  fftw_execute_dft((fftw_plan_s *)plan, reinterpret_cast<fftw_complex *>(in), reinterpret_cast<fftw_complex *>(out));
}

//Constructor which creates either a forward or reverse transform
FFTComplex::FFTComplex(int size, Complex *in, Complex *out, FFTComplex::fft_direction direction)
{
  if (in == out)
    throw FFTException("[BUG] In place transforms not supported");
  boost::mutex::scoped_lock lock(plannerMutex); //Lock the mutex
  if (direction == FFT_FORWARD)
    plan = fftw_plan_dft_1d(size, reinterpret_cast<fftw_complex *>(in), reinterpret_cast<fftw_complex *>(out), FFTW_FORWARD, FFTW_ESTIMATE);
  else
    plan = fftw_plan_dft_1d(size, reinterpret_cast<fftw_complex *>(in), reinterpret_cast<fftw_complex *>(out), FFTW_BACKWARD, FFTW_ESTIMATE);
  //Scoped lock automatically unlocks the mutex at the end of the scope
}

FFTComplex::~FFTComplex() {
  boost::mutex::scoped_lock lock(plannerMutex); //Lock the planner mutex
  fftw_destroy_plan(reinterpret_cast<fftw_plan>(plan));
  //Scoped lock unlocks mutex here
}

//
// Implementation of FFTExceptiom
//

/// Constructor
FFTException::FFTException(const std::string &str):
  std::runtime_error(str)
{
}
