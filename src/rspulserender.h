//rspulserender.h
//Definitions for pulse rendering functions
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//7 June 2006

#ifndef RS_PULSE_RENDER
#define RS_PULSE_RENDER

#include <config.h>
#include <queue>
#include <string>
#include <vector>

#include "rsradarwaveform.h"

//Forward definition of boost threads classes (see boost threads)
namespace boost
{
	class mutex;
}

namespace rs
{
	//Forward declaration of Response (see rsresponse.h)
	class Response;
	//Forward declaration of Receiver (see rsradar.h)
	class Receiver;


	/// Export the responses received by a receiver to an XML file
	void exportReceiverXml(const std::vector<rs::Response*>& responses, const std::string& filename);

	/// Export the receiver pulses to the specified binary file, using the specified quantization
	void exportReceiverBinary(const std::vector<rs::Response*>& responses, const rs::Receiver* recv,
	                          const std::string& recvName, const std::string& filename);

	/// Export the receiver responses to the specified CSV value files
	void exportReceiverCsv(const std::vector<rs::Response*>& responses, const std::string& filename);

	/// Management class for threaded rendering
	class ThreadedRenderer
	{
	public:
		/// Constructor
		ThreadedRenderer(const std::vector<rs::Response*>* responses, const rs::Receiver* recv, int maxThreads);

		/// Destructor
		~ThreadedRenderer();

		/// Render all the responses in a single window
		void renderWindow(RsComplex* window, rsFloat length, rsFloat start, rsFloat fracDelay) const;

	private:
		const std::vector<rs::Response*>* _responses; //!< Vector of target responses seen by this receiver
		const rs::Receiver* _recv; //!< Receiver we are rendering for
		int _max_threads; //!< The maximum allowed thread count for rendering
	};

	/// Single thread for rendering
	class RenderThread
	{
	public:
		/// Constructor
		RenderThread(int serial, boost::mutex* windowMutex, RsComplex* window, rsFloat length, rsFloat start,
		             rsFloat fracDelay, boost::mutex* workListMutex, std::queue<Response*>* workList);

		/// Destructor
		~RenderThread();

		/// Step through the worklist, rendering the required responses
		void operator()();

	private:
		/// Get a response from the worklist, returning NULL on failure
		Response* getWork() const;

		/// Add the array to the window, locking the window lock in advance
		void addWindow(const RsComplex* array, rsFloat startTime, unsigned int arraySize) const;

		int _serial; //!< Serial number of this thread
		boost::mutex* _window_mutex; //!< Mutex to protect window
		RsComplex* _window; //!< Pointer to render window
		rsFloat _length; //!< Length of render window (seconds)
		rsFloat _start; //!< Start time of render window (seconds)
		rsFloat _frac_delay; //!< Fractional window start time (< 1 sample, samples)
		boost::mutex* _work_list_mutex; //!< Mutex to protect work list
		std::queue<Response*>* _work_list; //!< List of responses to render
		RsComplex* _local_window;
	};
}


#endif
