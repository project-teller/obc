#pragma once

#include <cassert>
#include <cmsis_os2.h>

namespace teller::hal {

/**
 * @brief API-compatible implementation of std::mutex using CMSIS-RTOSv2
 */
class mutex {

public:
    typedef osMutexId_t native_handle_type;

public:
    mutex()
    {
        handle = osMutexNew(nullptr);
        assert(handle != nullptr);
    }
    ~mutex()
    {
        osStatus_t status = osMutexDelete(handle);
        (void)(status); /* prevent a gcc unused variable warning in release builds */
        assert(status == osOK);
    }

    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    void lock()
    {
        osStatus_t status = osMutexAcquire(handle, osWaitForever);
        (void)(status); /* prevent a gcc unused variable warning in release builds */
        assert(status == osOK);
    }

    bool try_lock()
    {
        osStatus_t status = osMutexAcquire(handle, 0U);
        return status == osOK;
    }

    void unlock()
    {
        osStatus_t status = osMutexRelease(handle);
        (void)(status); /* prevent a gcc unused variable warning in release builds */
        assert(status == osOK);
    }

private:
    native_handle_type handle;
};

/**
 * @brief API-compatible implementation of std::lock_guard using CMSIS-RTOSv2
 */
template <typename T = mutex>
class lock_guard {
public:
    typedef T mutex_type;

    explicit lock_guard(T& m)
        : _mutex(m)
    {
        _mutex.lock();
    }

    ~lock_guard()
    {
        _mutex.unlock();
    }

    lock_guard(const lock_guard&) = delete;
    lock_guard& operator=(const lock_guard&) = delete;

private:
    T& _mutex;
};

}
