//
// Created by David Young on 10/10/24.
//

#include "thread_pool.h"

#include "logging.h"

namespace pool
{
	ThreadPool::ThreadPool(const unsigned numThreads)
	{
		_workers.reserve(numThreads);
		for (unsigned i = 0; i < numThreads; ++i)
		{
			_workers.emplace_back([this]
			{
				while (true)
				{
					Task task;
					{
						std::unique_lock lock(_queue_mutex);
						_condition.wait(lock, [this] { return _stop || !_tasks.empty(); });
						if (_stop && _tasks.empty()) { return; }
						task = std::move(_tasks.front());
						_tasks.pop();
					}
					try { task(); }
					catch (const std::exception& e)
					{
						LOG(logging::Level::ERROR, "Exception in thread pool: {}", e.what());
					}

					// Decrement the pending tasks count and notify if all tasks are done
					{
						std::unique_lock lock(_queue_mutex);
						--_pending_tasks;
						if (_pending_tasks == 0) { _done_condition.notify_all(); }
					}
				}
			});
		}
	}

	ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock lock(_queue_mutex);
			_stop = true;
		}
		_condition.notify_all();
		for (std::thread& worker : _workers) { worker.join(); }
	}
}
