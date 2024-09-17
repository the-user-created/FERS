// response_renderer.h
// Definitions for response multi-threaded rendering classes
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 June 2006

#ifndef RESPONSE_RENDERER_H
#define RESPONSE_RENDERER_H

#include <mutex>
#include <complex>
#include <queue>
#include <vector>

#include "config.h"

namespace rs
{
	class Response;
	class Receiver;
}

namespace response_renderer
{
	class ThreadedResponseRenderer
	{
	public:
		ThreadedResponseRenderer(const std::vector<rs::Response*>* responses, const rs::Receiver* recv,
		                         const unsigned maxThreads) : _responses(responses), _recv(recv),
		                                                      _max_threads(maxThreads) {}

		~ThreadedResponseRenderer() = default;

		void renderWindow(RS_COMPLEX* window, RS_FLOAT length, RS_FLOAT start, RS_FLOAT fracDelay) const;

	private:
		const std::vector<rs::Response*>* _responses;
		const rs::Receiver* _recv;
		unsigned _max_threads;
	};

	class RenderThread
	{
	public:
		RenderThread(const int serial, std::mutex* windowMutex, RS_COMPLEX* window, const RS_FLOAT length,
		             const RS_FLOAT start, const RS_FLOAT fracDelay, std::mutex* workListMutex,
		             std::queue<rs::Response*>* workList) :
			_serial(serial), _window_mutex(windowMutex), _window(window), _length(length), _start(start),
			_frac_delay(fracDelay), _work_list_mutex(workListMutex), _work_list(workList) {}

		~RenderThread() = default;

		void operator()();

	private:
		[[nodiscard]] rs::Response* getWork() const;

		void addWindow(const RS_COMPLEX* array, RS_FLOAT startTime, unsigned arraySize) const;

		int _serial;
		std::mutex* _window_mutex;
		RS_COMPLEX* _window;
		RS_FLOAT _length;
		RS_FLOAT _start;
		RS_FLOAT _frac_delay;
		std::mutex* _work_list_mutex;
		std::queue<rs::Response*>* _work_list;
		RS_COMPLEX* _local_window{};
	};
}


#endif
