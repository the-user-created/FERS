// response_renderer.cpp
// Performs the second phase of the simulation - rendering the result
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 June 2006

#define TIXML_USE_STL

#include "response_renderer.h"

#include <thread>

#include "response.h"
#include "core/parameters.h"

namespace
{
	void addArrayToWindow(const RealType wStart, std::vector<ComplexType>& window, const unsigned wSize,
	                      const RealType rate,
	                      const RealType rStart, const std::vector<ComplexType>& resp, const unsigned rSize)
	{
		int start_sample = static_cast<int>(round(rate * (rStart - wStart)));
		unsigned roffset = 0;
		if (start_sample < 0)
		{
			roffset = -start_sample;
			start_sample = 0;
		}
		for (unsigned i = roffset; i < rSize && i + start_sample < wSize; i++) { window[i + start_sample] += resp[i]; }
	}
}

namespace serial
{
	// =================================================================================================================
	//
	// THREADED RENDERER CLASS
	//
	// =================================================================================================================

	void ThreadedResponseRenderer::renderWindow(std::vector<ComplexType>& window, const RealType length,
	                                            const RealType start,
	                                            const RealType fracDelay) const
	{
		const RealType end = start + length;
		std::queue<Response*> work_list;
		for (const auto& response : _responses)
		{
			const RealType resp_start = response->startTime();
			if (const RealType resp_end = response->endTime(); resp_start <= end && resp_end >= start)
			{
				work_list.push(response.get());
			}
		}

		std::mutex work_list_mutex, window_mutex;
		std::vector<std::thread> threads;
		{
			std::lock_guard lock(work_list_mutex);
			for (unsigned i = 0; i < _max_threads; i++)
			{
				threads.emplace_back([&, i]
				{
					const RenderThread thr(i, &window_mutex, window, length, start, fracDelay, &work_list_mutex,
					                       &work_list);
					thr();
				});
			}
		}

		for (auto& th : threads) { if (th.joinable()) { th.join(); } }
	}

	// =================================================================================================================
	//
	// RENDER THREAD CLASS
	//
	// =================================================================================================================

	void RenderThread::operator()() const
	{
		const RealType rate = params::rate() * params::oversampleRatio();
		const auto size = static_cast<unsigned>(std::ceil(_length * rate));
		std::vector local_window(size, ComplexType{});
		const Response* resp = getWork();
		while (resp)
		{
			unsigned psize;
			RealType prate;
			std::vector<ComplexType> array = resp->renderBinary(prate, psize, _frac_delay);
			addWindow(array, local_window, resp->startTime(), psize);
			resp = getWork();
		}
		{
			std::lock_guard lock(*_window_mutex);
			for (unsigned i = 0; i < size; i++) { _window[i] += local_window[i]; }
		}
	}

	void RenderThread::addWindow(const std::vector<ComplexType>& array, std::vector<ComplexType>& localWindow,
	                             const RealType startTime, const unsigned arraySize) const
	{
		const RealType rate = params::rate() * params::oversampleRatio();
		const auto size = static_cast<unsigned>(std::ceil(_length * rate));
		addArrayToWindow(_start, localWindow, size, rate, startTime, array, arraySize);
	}

	Response* RenderThread::getWork() const
	{
		Response* ret;
		{
			std::lock_guard lock(*_work_list_mutex);
			if (_work_list->empty()) { return nullptr; }
			ret = _work_list->front();
			_work_list->pop();
		}
		return ret;
	}
}
