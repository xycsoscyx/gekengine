/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Utility/String.hpp"
#include <concurrent_queue.h>
#include <coroutine>
#include <future>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gek
{
    struct TaskPromise;

    class Task
    {
    public:
        struct promise_type
        {
            Task get_return_object() noexcept
            {
                return { };
            };

            std::suspend_never initial_suspend() const noexcept
            {
                return { };
            }

            std::suspend_never final_suspend() const noexcept
            {
                return { };
            }

            void return_void() noexcept
            {
            }

            void unhandled_exception() noexcept
            {
                LockedWrite{ std::cerr } << "Unhandled Exception Occurred";
            }
        };
    };

    class ThreadPool final
    {
		using Lock = std::unique_lock<std::mutex>;
	
	private:
        uint32_t threadCount;
        std::vector<std::thread> workerList;
        concurrency::concurrent_queue<std::coroutine_handle<void>> coroutineQueue;

        std::mutex activeMutex;
        std::condition_variable activeCondition;
        std::atomic<bool> stop = false;
        std::atomic<uint32_t> activeCount = 0;

    private:
        void create(void)
        {
			stop.store(false);
            workerList.clear();
            workerList.reserve(threadCount);
            for (size_t count = 0; count < threadCount; ++count)
            {
                // Worker execution loop
                workerList.emplace_back([&](void) -> void
                {
#ifdef _WIN32
                    CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
#endif
					while (true)
                    {
						// Wait for additional work signal
                        // Wait to be notified of work

                        Lock lock(activeMutex);
                        activeCondition.wait(lock, [&](void) -> bool
                        {
                            return stop.load() || !coroutineQueue.empty();
                        });

                        // If stopping and no work remains, exit the work loop and thread
                        if (stop.load() && coroutineQueue.empty())
                        {
                            break;
                        }

						// Task to execute
                        std::coroutine_handle<void> coroutine;
						
						// Dequeue the next Task
                        if (coroutineQueue.try_pop(coroutine))
						{
							// Execute
                            coroutine.resume();
                        }

                        activeCount--;
                    };

#ifdef _WIN32
                    CoUninitialize();
#endif
                });
            }
        }

        void enqueue(std::coroutine_handle<void> coroutine)
        {
            if (stop.load())
            {
                return;
            }

            coroutineQueue.push(coroutine);
            activeCondition.notify_one();
        }

    public:
        ThreadPool(uint32_t threadCount)
            : threadCount(threadCount)
        {
            create();
        }

        // Destructor joins all worker threads
        ~ThreadPool(void)
        {
            drain();
        }

        ThreadPool(ThreadPool const &) = delete;
		ThreadPool(ThreadPool &&) = delete;
		
		ThreadPool& operator= (ThreadPool const &) = delete;
        ThreadPool& operator= (ThreadPool const &&) = delete;

        void join(void)
        {
            while (activeCount.load() > 0)
            {
                Sleep(0);
            };
        }

        void drain(bool executePendingTasks = true)
        {
			if (executePendingTasks)
			{
                join();
			}
			else
			{
				coroutineQueue.clear();
			}

            if (!workerList.empty())
            {
				stop.store(true);
                activeCondition.notify_all();

                // Wait for threads to complete work
                for (std::thread &worker : workerList)
                {
                    worker.join();
                }

                workerList.clear();
            }

            activeCount = 0;
        }
        
        void reset(void)
        {
            drain();
            create();
        }

        auto schedule()
        {
            activeCount++;
            struct Awaiter
            {
                ThreadPool *threadPool;

                constexpr bool await_ready() const noexcept
                {
                    return false;
                }
                
                constexpr void await_resume() const noexcept
                {
                }

                void await_suspend(std::coroutine_handle<> coroutine) const noexcept
                {
                    threadPool->enqueue(coroutine);
                }

                void return_void() noexcept
                {
                }
            };

            return Awaiter{ this };
        }
    };
}; // namespace Gek
