#pragma once

#include <cassert>
#include <cmsis_os2.h>

namespace teller::hal {

/**
 * @brief Generic size-limited queue implementation for message passing between tasks.
 *
 * On FreeRTOS, the implementation uses the CMSIS queue API.
 */
template <typename T>
class BlockingQueue : public BlockingQueueBase {

public:
    BlockingQueue(size_t size)
        : BlockingQueueBase(size)
    {
        handle = osMessageQueueNew(size, sizeof(T), nullptr);
        assert(handle != nullptr);
    }
    ~BlockingQueue()
    {
        assert(close());
    }

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    bool clear()
    {
        return osMessageQueueReset(handle) == osOK;
    }

    bool close()
    {
        if (is_closed) {
            return true;
        }

        is_closed = true;
        return osMessageQueueDelete(handle) == osOK;
    }

    size_t size() const { return osMessageQueueGetCount(handle); }

    bool receive(T& message)
    {
        if (is_closed) {
            return false;
        }

        return osMessageQueueGet(handle, &message, nullptr, osWaitForever) == osOK;
    }

    bool send(const T& message)
    {
        return send_with_timeout(message, osWaitForever);
    }

    bool send_or_drop(const T& message)
    {
        return send_with_timeout(message, 0);
    }

    bool send_with_timeout(const T& message, uint32_t timeout)
    {
        if (is_closed) {
            return false;
        }

        return osMessageQueuePut(handle, &message, 0, timeout) == osOK;
    }

private:
    /** FreeRTOS handle to the underlying queue */
    osMessageQueueId_t handle;
};

}
