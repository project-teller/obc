#pragma once

#include <cstdint>

namespace teller::drivers {

enum StorageOperation {
    OP_UNCONFIGURED,
    OP_UNMOUNTED,
    OP_UNKNOWN,
    OP_IDLE,
    OP_READ,
    OP_WRITE,
    OP_ERASE,
    OP_SYNC,
    OP_ERROR,
};

typedef struct {
    std::uint32_t bytesRead;
    std::uint32_t bytesWritten;
    std::uint32_t blocksErased;
} StorageStatistics;

/**
 * @brief RAII context manager that sets an operation variable to a value and
 * restores it upon exiting the context.
 */
class OperationContext {

public:
    OperationContext(StorageOperation* op, StorageOperation value)
        : _op(op)
    {
        _oldValue = op ? *op : OP_UNKNOWN;
        if (op) {
            *op = value;
        }
    }

    ~OperationContext()
    {
        if (_op) {
            *_op = _oldValue;
        }
    }

private:
    StorageOperation* _op;
    StorageOperation _oldValue;
};

}
