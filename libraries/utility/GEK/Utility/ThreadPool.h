#pragma once

#include <future>
#include <queue>

namespace Gek
{
    // https://github.com/progschj/ThreadPool
    class ThreadPool
    {
    private:
        // need to keep track of threads so we can join them
        std::vector<std::thread> workerList;

        // the task queue
        std::queue<std::function<void(void)>> taskQueue;

        // synchronization
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;

    public:
        // the constructor just launches some amount of workerList
        inline ThreadPool(size_t threadCount)
            : stop(false)
        {
            for (size_t thread = 0; thread < threadCount; ++thread)
            {
                workerList.emplace_back([this]
                {
                    for (;;)
                    {
                        std::function<void()> task;
                        if (true)
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock, [this]
                            {
                                return this->stop || !this->taskQueue.empty();
                            });

                            if (this->stop && this->taskQueue.empty())
                            {
                                return;
                            }

                            task = std::move(this->taskQueue.front());
                            this->taskQueue.pop();
                        }

                        task();
                    }
                });
            }
        }

        // the destructor joins all threads
        inline ~ThreadPool(void)
        {
            if (true)
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }

            condition.notify_all();
            for (std::thread &worker : workerList)
            {
                worker.join();
            }
        }

        void clear(void)
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            while (!taskQueue.empty())
            {
                taskQueue.pop();
            };
        }

        // add new work item to the pool
        template<class FUNCTION, class... PARAMETERS>
        auto enqueue(FUNCTION&& function, PARAMETERS&&... arguments) -> std::future<typename std::result_of<FUNCTION(PARAMETERS...)>::type>
        {
            using return_type = typename std::result_of<FUNCTION(PARAMETERS...)>::type;
            auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<FUNCTION>(function), std::forward<PARAMETERS>(arguments)...));
            std::future<return_type> future = task->get_future();
            if(true)
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                // don't allow enqueueing after stopping the pool
                if (stop)
                {
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                }

                taskQueue.emplace([task](void)
                {
                    (*task)();
                });
            }

            condition.notify_one();
            return future;
        }

        // add new work item to the pool
        void enqueue(std::function<void(void)> &&task)
        {
            if(true)
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                // don't allow enqueueing after stopping the pool
                if (stop)
                {
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                }

                taskQueue.emplace(std::move(task));
            }

            condition.notify_one();
        }
    };
}; // namespace Gek
