// response_renderer.cpp

#include "response_renderer.h"

#include <cmath>
#include <future>
#include <queue>
#include <thread>

#include "response.h"
#include "core/parameters.h"

namespace serial
{
	ThreadedResponseRenderer::ThreadedResponseRenderer(
		const std::span<const std::unique_ptr<Response>> responses, const radar::Receiver* recv,
		const unsigned maxThreads)
		: _responses(responses), _recv(recv), _max_threads(maxThreads) {}

	void ThreadedResponseRenderer::renderWindow(std::vector<ComplexType>& window, const RealType length,
	                                            const RealType start,
	                                            const RealType fracDelay) const
	{
		const RealType end = start + length;
		std::queue<Response*> work_list;

		// Populate the work list
		for (const auto& response : _responses)
		{
			const RealType resp_start = response->startTime();
			if (const RealType resp_end = response->endTime(); resp_start <= end && resp_end >= start)
			{
				work_list.push(response.get());
			}
		}

		// Shared resources
		std::mutex window_mutex, work_list_mutex;

		// Use a lambda to represent the worker logic, which simplifies the RenderThread class
		auto worker = [&](unsigned)
		{
			const RealType rate = params::rate() * params::oversampleRatio();
			auto local_window_size = static_cast<unsigned>(std::ceil(length * rate));
			std::vector local_window(local_window_size, ComplexType{});

			// Work processing loop
			while (true)
			{
				std::optional<Response*> resp_opt;
				{
					std::scoped_lock lock(work_list_mutex);
					if (work_list.empty()) { break; }
					resp_opt = work_list.front();
					work_list.pop();
				}

				const auto* resp = resp_opt.value();
				unsigned psize;
				RealType prate;
				std::vector<ComplexType> array = resp->renderBinary(prate, psize, fracDelay);
				int start_sample = static_cast<int>(std::round(rate * (resp->startTime() - start)));
				const unsigned roffset = start_sample < 0 ? -start_sample : 0;
				if (start_sample < 0) { start_sample = 0; }
				for (unsigned i = roffset; i < psize && i + start_sample < local_window_size; ++i)
				{
					local_window[i + start_sample] += array[i];
				}
			}

			// Merge local window into the shared window
			{
				std::scoped_lock lock(window_mutex);
				for (unsigned i = 0; i < local_window_size; ++i) { window[i] += local_window[i]; }
			}
		};

		// Create a pool of threads using std::async
		std::vector<std::future<void>> futures;
		for (unsigned i = 0; i < _max_threads; ++i) { futures.push_back(std::async(std::launch::async, worker, i)); }

		// Wait for all threads to finish
		for (auto& future : futures) { future.get(); }
	}
}
