// rspulserender.h
// Definitions for pulse rendering functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 June 2006

// TODO: This should be split into multiple files

#ifndef RS_PULSE_RENDER
#define RS_PULSE_RENDER

#include <queue>
#include <string>
#include <vector>

#include "config.h"
#include "radar_signal.h"

namespace boost
{
	class mutex; // NOLINT
}

namespace rs
{
	class Response;
	class Receiver;

	// TODO: These export functions should be moved to a separate file
	void exportReceiverXml(const std::vector<Response*>& responses, const std::string& filename);

	void exportReceiverBinary(const std::vector<Response*>& responses, const Receiver* recv,
	                          const std::string& recvName);

	void exportReceiverCsv(const std::vector<Response*>& responses, const std::string& filename);

	class ThreadedRenderer
	{
	public:
		ThreadedRenderer(const std::vector<Response*>* responses, const Receiver* recv,
		                 const unsigned maxThreads) : _responses(responses), _recv(recv), _max_threads(maxThreads) {}

		~ThreadedRenderer() = default;

		void renderWindow(RS_COMPLEX* window, RS_FLOAT length, RS_FLOAT start, RS_FLOAT fracDelay) const;

	private:
		const std::vector<Response*>* _responses;
		const Receiver* _recv;
		unsigned _max_threads;
	};

	class RenderThread
	{
	public:
		RenderThread(const int serial, boost::mutex* windowMutex, RS_COMPLEX* window, const RS_FLOAT length,
		             const RS_FLOAT start, const RS_FLOAT fracDelay, boost::mutex* workListMutex,
		             std::queue<Response*>* workList) :
			_serial(serial), _window_mutex(windowMutex), _window(window), _length(length), _start(start),
			_frac_delay(fracDelay), _work_list_mutex(workListMutex), _work_list(workList) {}

		~RenderThread() = default;

		void operator()();

	private:
		[[nodiscard]] Response* getWork() const;

		void addWindow(const RS_COMPLEX* array, RS_FLOAT startTime, unsigned arraySize) const;

		int _serial;
		boost::mutex* _window_mutex;
		RS_COMPLEX* _window;
		RS_FLOAT _length;
		RS_FLOAT _start;
		RS_FLOAT _frac_delay;
		boost::mutex* _work_list_mutex;
		std::queue<Response*>* _work_list;
		RS_COMPLEX* _local_window{};
	};
}


#endif
