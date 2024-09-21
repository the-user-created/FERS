// response_renderer.h
// Definitions for response multi-threaded rendering classes
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 June 2006

#ifndef RESPONSE_RENDERER_H
#define RESPONSE_RENDERER_H

#include <complex>
#include <memory>
#include <queue>

#include "config.h"

namespace radar {
	class Receiver;
}

namespace serial
{
	class Response;

	class ThreadedResponseRenderer
	{
	public:
		ThreadedResponseRenderer(const std::vector<std::unique_ptr<Response>>& responses, const radar::Receiver* recv,
		                         const unsigned maxThreads) : _responses(responses), _recv(recv),
		                                                      _max_threads(maxThreads) {}

		~ThreadedResponseRenderer() = default;

		void renderWindow(std::vector<RS_COMPLEX>& window, RS_FLOAT length, RS_FLOAT start, RS_FLOAT fracDelay) const;

	private:
		const std::vector<std::unique_ptr<Response>>& _responses;
		const radar::Receiver* _recv;
		unsigned _max_threads;
	};

	class RenderThread
	{
	public:
		RenderThread(const unsigned serial, std::mutex* windowMutex, std::vector<RS_COMPLEX>& window,
		             const RS_FLOAT length, const RS_FLOAT start, const RS_FLOAT fracDelay, std::mutex* workListMutex,
		             std::queue<Response*>* workList) :
			_serial(serial), _window_mutex(windowMutex), _window(window), _length(length), _start(start),
			_frac_delay(fracDelay), _work_list_mutex(workListMutex), _work_list(workList) {}

		~RenderThread() = default;

		void operator()() const;

	private:
		[[nodiscard]] Response* getWork() const;

		void addWindow(const std::vector<RS_COMPLEX>& array, std::vector<RS_COMPLEX>& localWindow, RS_FLOAT startTime,
		               unsigned arraySize) const;

		unsigned _serial;
		std::mutex* _window_mutex;
		std::vector<RS_COMPLEX>& _window;
		RS_FLOAT _length;
		RS_FLOAT _start;
		RS_FLOAT _frac_delay;
		std::mutex* _work_list_mutex;
		std::queue<Response*>* _work_list;
	};
}


#endif
