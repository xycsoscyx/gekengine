/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include <future>
#include <queue>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gek
{
    // https://github.com/progschj/ThreadPool
    template <size_t threadCount = 1>
    class ThreadPool final
    {
        using Lock = std::unique_lock<std::mutex>;

    private:
        std::vector<std::thread> workerList;
		using Task = std::tuple<std::function<void()>, char const *, size_t>;
        std::queue<Task> taskQueue;

        std::mutex queueMutex;
        std::condition_variable condition;
		std::atomic<bool> stop = false;

	private:
        void create(void)
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
                    CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
#endif
					size_t lastTaskLine = 0;
					char const *lastTaskName = nullptr;
					for (;;)
                    {
						// Task to execute
						Task task;
						
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
						lastTaskName = std::get<1>(task);
						lastTaskLine = std::get<2>(task);
						std::get<0>(task)();
					}

#ifdef _WIN32
                    CoUninitialize();
#endif
                });
            }
        }

    public:
        ThreadPool(void)
        {
            create();
        }

        // Destructor joins all worker threads
        ~ThreadPool(void)
        {
            drain();
        }

        ThreadPool(const ThreadPool &) = delete;
		ThreadPool(ThreadPool &&) = delete;
		
		ThreadPool& operator= (const ThreadPool &) = delete;
        ThreadPool& operator= (const ThreadPool &&) = delete;

        void drain(void)
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
        
        void ThreadPool::reset(void)
        {
            drain();
            create();
        }

        template<typename FUNCTION, typename... PARAMETERS>
        auto enqueue(FUNCTION&& function, PARAMETERS&&... arguments, char const *fileName = nullptr, size_t line = 0) -> std::future<typename std::result_of<FUNCTION(PARAMETERS...)>::type>
        {
            using ReturnType = typename std::result_of<FUNCTION(PARAMETERS...)>::type;
            using PackagedTask = std::packaged_task<ReturnType()>;

            auto task = std::make_shared<PackagedTask>(std::bind(std::forward<FUNCTION>(function), std::forward<PARAMETERS>(arguments)...));
            std::future<ReturnType> result = task->get_future();

            if(true)
            {
                Lock lock(queueMutex);
                if (stop)
                {
                    return result;
                }

                taskQueue.emplace(std::make_tuple([task]()
                {
                    (*task)();
                }, fileName, line));
            }

            condition.notify_one();
            return result;
        }

        template<typename FUNCTION, typename... PARAMETERS>
        auto enqueueAndDetach(FUNCTION&& function, PARAMETERS&&... arguments, char const *fileName = nullptr, size_t line = 0) -> void
        {
            if(true)
            {
                Lock lock(queueMutex);
                if (stop)
                {
					return;
                }

                taskQueue.emplace(std::make_tuple(std::bind(std::forward<FUNCTION>(function), std::forward<PARAMETERS>(arguments)...), fileName, line));
            }

            condition.notify_one();
        }
    };
}; // namespace Gek
