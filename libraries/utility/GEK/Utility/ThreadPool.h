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
        // Keep track of threads, so they can be joined
        std::vector<std::thread> workerList;

        // Task queue
        std::queue<std::function<void()>> taskQueue;

        // Synchronization
        std::mutex queueMutex;
        std::condition_variable condition;
        bool stop = false;

    public:
        // Launches specified number of worker threads
        ThreadPool(size_t threadCount = 1);
        ~ThreadPool(void);

        // Not copyable
        ThreadPool(const ThreadPool &) = delete;
        ThreadPool& operator= (const ThreadPool &) = delete;

        // Not moveable
        ThreadPool(ThreadPool &&) = delete;
        ThreadPool& operator= (const ThreadPool &&) = delete;

        // Clear pending taskQueue
        void clear(void);

        // Enqueue task and return std::future<>
        template<typename FUNCTION, typename... PARAMETERS>
        auto enqueue(FUNCTION&& function, PARAMETERS&&... arguments) -> std::future<typename std::result_of<FUNCTION(PARAMETERS...)>::type>
        {
            using ReturnType = typename std::result_of<FUNCTION(PARAMETERS...)>::type;
            using PackagedTask = std::packaged_task<ReturnType()>;

            auto task = std::make_shared<PackagedTask>(std::bind(std::forward<FUNCTION>(function), std::forward<Args>(arguments)...));
            std::future<ReturnType> result = task->get_future();

            if(true)
            {
                Lock lock(queueMutex);

                // Don't allow an enqueue after stopping
                if (stop)
                {
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                }

                // Push work back on the queue
                taskQueue.emplace([task]()
                {
                    (*task)();
                });
            }

              // Notify a thread that there is new work to perform
            condition.notify_one();
            return result;
        }

        // Enqueue task without requiring capture of std::future<>
        // Note: Best not to let exceptions escape the function
        template<typename FUNCTION, typename... PARAMETERS>
        auto enqueueAndDetach(FUNCTION&& function, PARAMETERS&&... arguments) -> void
        {
            if(true)
            {
                Lock lock(queueMutex);

                // Don't allow an enqueue after stopping
                if (stop)
                {
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                }

                // Push work back on the queue
                taskQueue.emplace(std::bind(std::forward<FUNCTION>(function), std::forward<PARAMETERS>(arguments)...));
            }

              // Notify a thread that there is new work to perform
            condition.notify_one();
        }
    };
}; // namespace Gek
