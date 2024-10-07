// response_renderer.h
// Definitions for response multi-threaded rendering classes
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 June 2006

#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <queue>
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
		ThreadedResponseRenderer(const std::span<const std::unique_ptr<Response>> responses,
		                         const radar::Receiver* recv,
		                         const unsigned maxThreads) : _responses(responses), _recv(recv),
		                                                      _max_threads(maxThreads) {}

		~ThreadedResponseRenderer() = default;

		void renderWindow(std::vector<ComplexType>& window, RealType length, RealType start, RealType fracDelay) const;

	private:
		const std::span<const std::unique_ptr<Response>> _responses;
		const radar::Receiver* _recv;
		unsigned _max_threads;
	};

	class RenderThread
	{
	public:
		RenderThread(const unsigned serial, std::mutex& windowMutex, std::vector<ComplexType>& window,
		             const RealType length, const RealType start, const RealType fracDelay, std::mutex& workListMutex,
		             std::queue<Response*>& workList) noexcept : _serial(serial), _window_mutex(windowMutex),
		                                                         _window(window), _length(length), _start(start),
		                                                         _frac_delay(fracDelay),
		                                                         _work_list_mutex(workListMutex),
		                                                         _work_list(workList) {}

		~RenderThread() = default;

		void operator()() const;

	private:
		[[nodiscard]] std::optional<Response*> getWork() const noexcept;

		void addWindow(const std::vector<ComplexType>& array, std::vector<ComplexType>& localWindow, RealType startTime,
		               unsigned arraySize) const noexcept;

		unsigned _serial;
		std::mutex& _window_mutex;
		std::vector<ComplexType>& _window;
		RealType _length;
		RealType _start;
		RealType _frac_delay;
		std::mutex& _work_list_mutex;
		std::queue<Response*>& _work_list;
	};
}
