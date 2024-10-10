// response_renderer.h

#pragma once

#include <future>
#include <memory>
#include <span>
#include <vector>

#include "config.h"

namespace radar
{
	class Receiver;
}

namespace serial
{
	class Response;

	class ThreadedResponseRenderer
	{
	public:
		ThreadedResponseRenderer(std::span<const std::unique_ptr<Response>> responses, const radar::Receiver* recv,
		                         unsigned maxThreads);

		void renderWindow(std::vector<ComplexType>& window, RealType length, RealType start, RealType fracDelay) const;

	private:
		const std::span<const std::unique_ptr<Response>> _responses;
		const radar::Receiver* _recv;
		unsigned _max_threads;
	};
}
