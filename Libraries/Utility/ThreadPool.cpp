#include "GEK/Utility/ThreadPool.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gek
{
	void ThreadPool::drain(void)
	{
		[this](void)
		{
			Lock lock(queueMutex);
			while (!taskQueue.empty())
			{
				taskQueue.pop();
			};
		} ();

		if (!workerList.empty())
		{
			stop = true;
			condition.notify_all();

			// Wait for threads to complete work
			for (std::thread &worker : workerList)
			{
				worker.join();
			}

			workerList.clear();
		}
	}

	void ThreadPool::create(size_t threadCount)
	{
		stop = false;
		workerList.clear();
		workerList.reserve(threadCount);
		for (size_t count = 0; count < threadCount; ++count)
		{
			// Worker execution loop
			workerList.emplace_back([this](void) -> void
			{
#ifdef _WIN32
				CoInitialize(nullptr);
#endif
				for (;;)
				{
					// Task to execute
					std::function<void()> task;

					// Wait for additional work signal
					if (true)
					{
						// Wait to be notified of work
						Lock lock(queueMutex);
						condition.wait(lock, [this](void) -> bool
						{
							return stop || !taskQueue.empty();
						});

						// If stopping and no work remains, exit the work loop and thread
						if (stop && taskQueue.empty())
						{
							break;
						}

						// Dequeue the next task
						task = std::move(taskQueue.front());
						taskQueue.pop();
					}

					// Execute
					task();
				}

#ifdef _WIN32
				CoUninitialize();
#endif
			});
		}
	}

	ThreadPool::ThreadPool(size_t threadCount)
    {
		create(threadCount);
    }

    // Destructor joins all worker threads
    ThreadPool::~ThreadPool(void)
    {
		drain();
    }

    void ThreadPool::reset(size_t *threadCount)
    {
		drain();
		create(threadCount ? *threadCount : workerList.size());
	}
}; // namespace Gek
