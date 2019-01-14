#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <mutex>
#include <condition_variable>
#include <queue>

#define MAX_CAPACITY 300

namespace crawler
{

template<typename T>
class Block_queue
{
public:
    Block_queue()
        : capacity(MAX_CAPACITY)
    {
    }

    void push(const T& value)
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        full_v.wait(lock, [this] () {
            return queue.size() < capacity;
        });
        queue.push(value);
        empty_v.notify_all();
    }

    void push(T&& value)
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        full_v.wait(lock, [this] () {
            return queue.size() < capacity;
        });
        queue.push(std::move(value));
        empty_v.notify_all();
    }

    T& front()
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        empty_v.wait(lock, [this] () {
            return !queue.empty();
        });
        return queue.front();
    }

    void pop()
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        empty_v.wait(lock, [this] () {
            return !queue.empty();
        });
        queue.pop();
        full_v.notify_all();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return queue.empty();
    }

private:
    std::queue<T> queue;
    mutable std::mutex queue_mutex;
    std::condition_variable full_v;
    std::condition_variable empty_v;
    std::size_t capacity;

}; // class safe_queue
} // namespace crawler
#endif // BLOCK_QUEUE