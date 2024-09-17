// response_renderer.cpp
// Performs the second phase of the simulation - rendering the result
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 June 2006

#define TIXML_USE_STL

#include "response_renderer.h"

#include "core/parameters.h"
#include "core/portable_utils.h"
#include "radar/radar_system.h"

namespace
{
	void addArrayToWindow(const RS_FLOAT wStart, RS_COMPLEX* window, const unsigned wSize, const RS_FLOAT rate,
	                      const RS_FLOAT rStart, const RS_COMPLEX* resp, const unsigned rSize)
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

// =====================================================================================================================
//
// THREADED RENDERER CLASS
//
// =====================================================================================================================

namespace response_renderer
{
	void ThreadedResponseRenderer::renderWindow(RS_COMPLEX* window, const RS_FLOAT length, const RS_FLOAT start,
	                                            const RS_FLOAT fracDelay) const
	{
		const RS_FLOAT end = start + length;
		std::queue<rs::Response*> work_list;
		for (auto response : *_responses)
		{
			const RS_FLOAT resp_start = response->startTime();
			if (const RS_FLOAT resp_end = response->endTime(); resp_start <= end && resp_end >= start)
			{
				work_list.push(response);
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
					RenderThread thr(i, &window_mutex, window, length, start, fracDelay, &work_list_mutex, &work_list);
					thr();
				});
			}
		}

		for (auto& th : threads) { if (th.joinable()) { th.join(); } }
	}

	// =====================================================================================================================
	//
	// RENDER THREAD CLASS
	//
	// =====================================================================================================================

	void RenderThread::operator()()
	{
		const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
		const auto size = static_cast<unsigned>(std::ceil(_length * rate));
		_local_window = new RS_COMPLEX[size]();
		const rs::Response* resp = getWork();
		while (resp)
		{
			unsigned psize;
			RS_FLOAT prate;
			std::shared_ptr<RS_COMPLEX[]> array = resp->renderBinary(prate, psize, _frac_delay);
			addWindow(array.get(), resp->startTime(), psize);
			resp = getWork();
		}
		{
			std::lock_guard lock(*_window_mutex);
			for (unsigned i = 0; i < size; i++) { _window[i] += _local_window[i]; }
		}
		delete[] _local_window;
	}

	void RenderThread::addWindow(const RS_COMPLEX* array, const RS_FLOAT startTime, const unsigned arraySize) const
	{
		const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
		const auto size = static_cast<unsigned>(std::ceil(_length * rate));
		addArrayToWindow(_start, _local_window, size, rate, startTime, array, arraySize);
	}

	rs::Response* RenderThread::getWork() const
	{
		rs::Response* ret;
		{
			std::lock_guard lock(*_work_list_mutex);
			if (_work_list->empty()) { return nullptr; }
			ret = _work_list->front();
			_work_list->pop();
		}
		return ret;
	}
}
