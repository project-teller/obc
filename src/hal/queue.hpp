#pragma once

#include "config.h"
#include <cstddef>
#include <stdexcept>

namespace teller::hal {

class BlockingQueueBase {
public:
    BlockingQueueBase(size_t size)
        : is_closed(false)
        , item_limit(size)
    {
        if (size == 0) {
            throw std::runtime_error("zero-length queues are not allowed");
        }
    }
    virtual ~BlockingQueueBase() { }

    bool closed() const { return is_closed; }

    virtual bool close() = 0;
    bool empty() const { return size() == 0; }
    virtual size_t size() const = 0;
    virtual size_t limit() const
    {
        return item_limit;
    }

protected:
    /** Whether the queue is closed */
    bool is_closed;

    /** Size limit of the queue */
    const size_t item_limit;
};

}

#ifdef TELLER_BOARD_POSIX
#include "hal/posix/queue.hpp"
#else
#include "hal/stm32/queue.hpp"
#endif
