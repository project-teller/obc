#pragma once

#include <cassert>
#include <condition_variable>
#include <mutex>

// For compatibility with CMSIS-RTOSv2
static const uint32_t osFlagsErrorParameter = 0xFFFFFFFCU;

// For compatibility with CMSIS-RTOSv2
static const uint32_t osWaitForever = 0xFFFFFFFFU;

namespace teller::hal {

/**
 * @brief POSIX emulation of the CMSIS-RTOS event flags functionality.
 */
class EventFlags {
public:
    EventFlags()
        : current(0)
    {
    }

    EventFlags(const EventFlags&) = delete;
    EventFlags& operator=(const EventFlags&) = delete;

    uint32_t clear(uint32_t flags)
    {
        std::unique_lock lock(mutex);
        uint32_t old = current;

        if (!checkValidFlags(flags)) {
            return osFlagsErrorParameter;
        }

        current &= ~flags;
        if (current != old) {
            changed_event.notify_all();
        }

        return old;
    }

    uint32_t get() const
    {
        std::unique_lock lock(mutex);
        return current;
    }

    uint32_t set(uint32_t flags)
    {
        std::unique_lock lock(mutex);
        uint32_t old = current;

        if (!checkValidFlags(flags)) {
            return osFlagsErrorParameter;
        }

        current |= flags;
        if (current != old) {
            changed_event.notify_all();
        }

        return current;
    }

    uint32_t waitAll(uint32_t flags, uint32_t timeout = osWaitForever)
    {
        std::unique_lock lock(mutex);
        uint32_t old;

        if (!checkValidFlags(flags)) {
            return osFlagsErrorParameter;
        }

        while ((current & flags) != flags) {
            if (timeout == osWaitForever) {
                changed_event.wait(lock);
            } else {
                changed_event.wait_for(lock, std::chrono::milliseconds(timeout));
            }
        }

        old = current;
        current &= ~flags;

        return old;
    }

    uint32_t waitAny(uint32_t flags, uint32_t timeout = osWaitForever)
    {
        std::unique_lock lock(mutex);
        uint32_t old;

        if (!checkValidFlags(flags)) {
            return osFlagsErrorParameter;
        }

        while ((current & flags) == 0) {
            if (timeout == osWaitForever) {
                changed_event.wait(lock);
            } else {
                changed_event.wait_for(lock, std::chrono::milliseconds(timeout));
            }
        }

        old = current;
        current &= ~flags;

        return old;
    }

private:
    /** Current values of the event flags */
    uint32_t current;

    /** Mutex for synchronization */
    mutable std::mutex mutex;

    /** Event to notify listeners about changes */
    std::condition_variable changed_event;

    bool checkValidFlags(uint32_t flags)
    {
        return (flags & 0x80000000U) == 0;
    }
};
}
