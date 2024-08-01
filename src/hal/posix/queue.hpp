#pragma once

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace teller::hal {

/**
 * @brief Generic size-limited queue implementation for message passing between tasks.
 *
 * On POSIX, the implementation uses an in-memory buffer protected by a mutex.
 */
template <typename T>
class BlockingQueue {

public:
    BlockingQueue(size_t size)
        : item_limit(size)
        , is_closed(false)
    {
        if (size == 0) {
            throw std::runtime_error("zero-length queues are not allowed");
        }
    }

    ~BlockingQueue()
    {
        close();
    }

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    bool clear()
    {
        std::unique_lock lock(queue_mutex);

        while (!items.empty()) {
            items.pop();
            item_removed_event.notify_one();
        }

        return true;
    }

    bool close()
    {
        if (!is_closed) {
            is_closed = true;
            item_added_event.notify_all();
        }

        return true;
    }

    bool closed() const { return is_closed; }
    bool empty() const
    {
        std::unique_lock lock(queue_mutex);
        return items.empty();
    }
    size_t size() const
    {
        std::unique_lock lock(queue_mutex);
        return items.size();
    }
    size_t limit() const { return item_limit; }

    bool receive(T& message)
    {
        std::unique_lock lock(queue_mutex);

        while (items.empty()) {
            if (is_closed) {
                return false;
            }
            item_added_event.wait(lock);
        }

        message = items.front();

        items.pop();
        item_removed_event.notify_one();

        return true;
    }

    bool send(const T& message)
    {
        std::unique_lock lock(queue_mutex);

        if (is_closed) {
            return false;
        }

        while (items.size() >= item_limit) {
            item_removed_event.wait(lock);
        }

        items.push(message);
        item_added_event.notify_one();

        return true;
    }

    bool send_or_drop(const T& message)
    {
        std::unique_lock lock(queue_mutex);

        if (is_closed || items.size() >= item_limit) {
            return false;
        }

        items.push(message);
        item_added_event.notify_one();

        return true;
    }

    bool send_with_timeout(const T& message, uint32_t timeout)
    {
        if (timeout <= 0) {
            return send_or_drop(message);
        }

        std::unique_lock lock(queue_mutex);

        if (is_closed) {
            return false;
        }

        while (items.size() >= item_limit) {
            item_removed_event.wait_for(lock, std::chrono::milliseconds(timeout));
            if (!lock.owns_lock()) {
                return false;
            }
        }

        items.push(message);
        item_added_event.notify_one();

        return true;
    }

private:
    /** The queue in which the elements are stored */
    std::queue<T> items;

    /** Mutex for synchronization */
    mutable std::mutex queue_mutex;

    /** Size limit of the queue */
    const size_t item_limit;

    /** Whether the queue is closed */
    bool is_closed;

    std::condition_variable item_added_event;
    std::condition_variable item_removed_event;
};
}
