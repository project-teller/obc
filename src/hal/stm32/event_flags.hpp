#pragma once

#include <cassert>
#include <cmsis_os2.h>
#include <cstdint>

namespace teller::hal {

/**
 * @brief Object oriented wrapper around CMSIS-RTOS event flags.
 */
class EventFlags {
public:
    EventFlags()
        : is_disposed(false)
    {
        handle = osEventFlagsNew(nullptr);
        assert(handle != nullptr);
    }

    ~EventFlags()
    {
        assert(dispose());
    }

    EventFlags(const EventFlags&) = delete;
    EventFlags& operator=(const EventFlags&) = delete;

    uint32_t clear(uint32_t flags)
    {
        return osEventFlagsClear(handle, flags);
    }

    uint32_t get() const
    {
        return osEventFlagsGet(handle);
    }

    uint32_t set(uint32_t flags)
    {
        return osEventFlagsSet(handle, flags);
    }

    uint32_t waitAll(uint32_t flags, uint32_t timeout = osWaitForever)
    {
        return osEventFlagsWait(handle, flags, osFlagsWaitAll, timeout);
    }

    uint32_t waitAny(uint32_t flags, uint32_t timeout = osWaitForever)
    {
        return osEventFlagsWait(handle, flags, osFlagsWaitAny, timeout);
    }

    /** Constant denoting an event set that contains all events */
    static const uint32_t ALL_EVENTS = 0x7FFFFFFF;

private:
    /** FreeRTOS handle to the underlying event flags */
    osEventFlagsId_t handle;

    /** Whether the event flags object has already been disposed of */
    bool is_disposed;

    bool dispose()
    {
        if (is_disposed) {
            return true;
        }

        is_disposed = true;
        return osEventFlagsDelete(handle) == osOK;
    }
};

}
