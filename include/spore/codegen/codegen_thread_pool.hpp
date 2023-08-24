#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "concurrentqueue/concurrentqueue.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

namespace spore::codegen
{
    struct codegen_thread_pool
    {
        std::mutex mutex;
        std::condition_variable condition;
        std::vector<std::thread> threads;
        std::atomic<bool> ending = false;
        moodycamel::ConcurrentQueue<std::function<void()>> tasks;

        inline explicit codegen_thread_pool(std::size_t size)
        {
            threads.resize(size);

            for (std::thread& thread : threads)
            {
                const auto action = [this] {
                    _thread_execute_tasks();
                };

                thread = std::thread(action);
            }
        }

        inline ~codegen_thread_pool()
        {
            end_execution();
        }

        inline void end_execution()
        {
            if (!ending)
            {
                ending = true;

                condition.notify_all();

                for (std::thread& thread : threads)
                {
                    thread.join();
                }
            }
        }

        inline void add_task(std::function<void()> task)
        {
            tasks.enqueue(std::move(task));

            condition.notify_one();
        }

        inline void join()
        {
        }

        inline void _thread_execute_tasks()
        {
            moodycamel::ConsumerToken consumer(tasks);

            while (!ending)
            {
                std::function<void()> task;

                if (tasks.try_dequeue(consumer, task))
                {
                    _thread_execute_one_task(task);
                }
                else if (!ending)
                {
                    std::unique_lock lock(mutex);

                    condition.wait(lock);
                }
            }
        }

        static inline void _thread_execute_one_task(const std::function<void()>& task)
        {
            try
            {
                // memory fence to ensure the closure has
                // access to the latest memory transactions

                std::atomic_thread_fence(std::memory_order_acquire);
                task();
            }
            catch (...)
            {
                SPDLOG_ERROR("unhandled exception in async task");
            }
        }
    };
}