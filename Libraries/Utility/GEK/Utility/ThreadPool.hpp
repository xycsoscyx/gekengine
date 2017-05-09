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

namespace Gek
{
    // https://github.com/progschj/ThreadPool
    class ThreadPool final
    {
        using Lock = std::unique_lock<std::mutex>;

    private:
        std::vector<std::thread> workerList;
        std::queue<std::function<void()>> taskQueue;

        std::mutex queueMutex;
        std::condition_variable condition;
		std::atomic<bool> stop = false;

	private:
		void create(size_t threadCount);

    public:
        ThreadPool(size_t threadCount = 1);
        ~ThreadPool(void);

        ThreadPool(const ThreadPool &) = delete;
		ThreadPool(ThreadPool &&) = delete;
		
		ThreadPool& operator= (const ThreadPool &) = delete;
        ThreadPool& operator= (const ThreadPool &&) = delete;

		void drain(void);
		void reset(size_t *threadCount = nullptr);

        template<typename FUNCTION, typename... PARAMETERS>
        auto enqueue(FUNCTION&& function, PARAMETERS&&... arguments) -> std::future<typename std::result_of<FUNCTION(PARAMETERS...)>::type>
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
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                }

                taskQueue.emplace([task]()
                {
                    (*task)();
                });
            }

            condition.notify_one();
            return result;
        }

        template<typename FUNCTION, typename... PARAMETERS>
        auto enqueueAndDetach(FUNCTION&& function, PARAMETERS&&... arguments) -> void
        {
            if(true)
            {
                Lock lock(queueMutex);
                if (stop)
                {
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                }

                taskQueue.emplace(std::bind(std::forward<FUNCTION>(function), std::forward<PARAMETERS>(arguments)...));
            }

            condition.notify_one();
        }
    };
}; // namespace Gek
