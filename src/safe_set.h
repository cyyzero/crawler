#ifndef SAFE_SET
#define SAFE_SET

#include <mutex>
#include <utility>
#include <unordered_set>

namespace crawler
{

template<typename T>
class Safe_set
{

public:
    using iterator       = typename std::unordered_set<T>::iterator;
    using const_iterator = typename std::unordered_set<T>::const_iterator;

    template<typename... Args>
    Safe_set(Args... args)
        : set(std::forward<Args>(args)...)
    {
    }

    ~Safe_set() = default;

    iterator find(const T& value)
    {
        std::lock_guard<std::mutex> lock(set_mutex);
        return set.find(value);
    }

    const_iterator find(const T& value) const
    {
        std::lock_guard<std::mutex> lock(set_mutex);
        return set.find(value);
    }

    iterator end() noexcept
    {
        std::lock_guard<std::mutex> lock(set_mutex);
        return set.end();
    }

    const_iterator end() const noexcept
    {
        std::lock_guard<std::mutex> lock(set_mutex);
        return set.end();
    }

    std::pair<iterator, bool> insert(const T& value)
    {
        std::lock_guard<std::mutex> lock(set_mutex);
        return set.insert(value);
    }

    std::pair<iterator, bool> insert(T&& value)
    {
        std::lock_guard<std::mutex> lock(set_mutex);
        return set.insert(std::move(value));
    }

private:
    std::unordered_set<T> set;
    mutable std::mutex set_mutex;
}; // class safe_set

} // namespace crawler

#endif