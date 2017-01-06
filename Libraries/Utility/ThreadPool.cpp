#include "GEK/Utility/ThreadPool.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gek
{
    ThreadPool::ThreadPool(size_t threadCount)
    {
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
                        Lock lock(this->queueMutex);
                        this->condition.wait(lock, [this](void) -> bool
                        {
                            return this->stop || !this->taskQueue.empty();
                        });

                        // If stopping and no work remains, exit the work loop and thread
                        if (this->stop && this->taskQueue.empty())
                        {
                            break;
                        }

                        // Dequeue the next task
                        task = std::move(this->taskQueue.front());
                        this->taskQueue.pop();
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

    // Destructor joins all worker threads
    ThreadPool::~ThreadPool(void)
    {
        if(true)
        {
            Lock lock(queueMutex);
            stop = true;
        }

        condition.notify_all();

        // Wait for threads to complete work
        for (std::thread &worker : workerList)
        {
            worker.join();
        }
    }

    void ThreadPool::clear(void)
    {
        Lock lock(queueMutex);
        while (!taskQueue.empty())
        {
            taskQueue.pop();
        };
    }
}; // namespace Gek
