#ifndef THREAD_POOL
#define THREAD_POOL

#include <atomic>
#include <thread>
#include <mutex>
#include <array>
#include <queue>
#include <functional>
#include <condition_variable>

namespace crawler
{

template<unsigned ThreadCount = 10>
class thread_pool
{

public:
    thread_pool()
        : jobs_left(0), bailout(false), finished(false)
    {
        for (unsigned i = 0; i < ThreadCount; ++i)
        {
            threads[i] = std::thread([this] () { this->task(); });
        }
    }

    ~thread_pool()
    {
        join_all();
    }

    unsigned size() const
    {
        return ThreadCount;
    }

    unsigned jobs_remaining()
    {
        std::lock_guard<std::mutex> guard(queue_mutex);
        return queue.size();
    }

    void add_job(std::function<void()> job)
    {
        std::lock_guard<std::mutex> guard(queue_mutex);
        queue.push(job);
        ++jobs_left;
        job_available_var.notify_one();
    }

    void join_all(bool wait_for_all = true)
    {
        if (!finished)
        {
            if (wait_for_all)
            {
                wait_all();
            }
        }

        bailout = true;
        job_available_var.notify_all();

        for (auto &x : threads)
        {
            if (x.joinable())
                x.join();
        }

        finished = true;
    }

    void wait_all()
    {
        if (jobs_left > 0)
        {
            std::unique_lock<std::mutex> lock(wait_mutex);
            wait_var.wait(lock, [this] () {
                return jobs_left == 0;
            });
            lock.unlock();
        }
    }

private:
    void task()
    {
        while (!bailout)
        {
            next_job()();
            --jobs_left;
            wait_var.notify_one();
        }
    }

    std::function<void()> next_job()
    {
        std::function<void()> res;
        std::unique_lock<std::mutex> job_lock(queue_mutex);

        job_available_var.wait(job_lock, [this]() -> bool {
            return queue.size() || bailout;
        });

        if (!bailout)
        {
            res = queue.front();
            queue.pop();
        }
        else
        {
            res = [] () {};
            ++jobs_left;
        }
        return res;
    }

private:
    std::array<std::thread, ThreadCount> threads;
    std::queue<std::function<void()>> queue;

    std::atomic_int         jobs_left;
    std::atomic_bool        bailout;
    std::atomic_bool        finished;
    std::mutex              wait_mutex;
    std::mutex              queue_mutex;
    std::condition_variable wait_var;
    std::condition_variable job_available_var;

}; // class thread_pool

}; // namespace crawler

#endif // THREAD_POOL