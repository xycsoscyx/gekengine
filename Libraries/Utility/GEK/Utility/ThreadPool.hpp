/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include <future>
#include <concurrent_queue.h>

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
        concurrency::concurrent_queue<Task> taskQueue;

		std::mutex mutex;
        std::condition_variable condition;
		std::atomic<bool> stop = false;

	private:
        void create(void)
        {
			stop.store(false);
            workerList.clear();
            workerList.reserve(threadCount);
            for (size_t count = 0; count < threadCount; ++count)
            {
				static size_t lastTaskLine = 0;
				static char const *lastTaskName = nullptr;
				// Worker execution loop
                workerList.emplace_back([this](void) -> void
                {
#ifdef _WIN32
                    CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
#endif
					while (true)
                    {
						// Wait for additional work signal
                        // Wait to be notified of work
                        condition.wait(Lock(mutex), [this](void) -> bool
                        {
                            return stop.load() || !taskQueue.empty();
                        });

                        // If stopping and no work remains, exit the work loop and thread
                        if (stop.load() && taskQueue.empty())
                        {
                            break;
                        }

						// Task to execute
						Task task;
						
						// Dequeue the next task
						if (taskQueue.try_pop(task))
						{
							// Execute
							lastTaskName = std::get<1>(task);
							lastTaskLine = std::get<2>(task);
							std::get<0>(task)();
						}
					};

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

        ThreadPool(ThreadPool const &) = delete;
		ThreadPool(ThreadPool &&) = delete;
		
		ThreadPool& operator= (ThreadPool const &) = delete;
        ThreadPool& operator= (ThreadPool const &&) = delete;

        void drain(bool executePendingTasks = false)
        {
            [this, executePendingTasks](void)
            {
                while (!taskQueue.empty())
                {
					Task task;
					if (taskQueue.try_pop(task) && executePendingTasks)
					{
						std::get<0>(task)();
					}
                };
            } ();

            if (!workerList.empty())
            {
				stop.store(true);
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

        template<typename FUNCTION>
        auto enqueue(FUNCTION&& function, char const *fileName = nullptr, size_t line = 0) -> std::future<typename std::result_of<FUNCTION(void)>::type>
        {
            using ReturnType = typename std::result_of<FUNCTION(void)>::type;
            using PackagedTask = std::packaged_task<ReturnType()>;

            auto task = std::make_shared<PackagedTask>(std::forward<FUNCTION>(function));
            std::future<ReturnType> result = task->get_future();

			if (stop.load())
			{
                return result;
            }

            taskQueue.push(std::make_tuple([task]()
            {
                (*task)();
            }, fileName, line));
            condition.notify_one();
            return result;
        }

        template<typename FUNCTION>
        void enqueueAndDetach(FUNCTION&& function, char const *fileName = nullptr, size_t line = 0)
        {
            if (stop.load())
            {
				return;
            }

            taskQueue.push(std::make_tuple(std::forward<FUNCTION>(function), fileName, line));
            condition.notify_one();
        }
    };
}; // namespace Gek
