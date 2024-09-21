#include "hal/memory.h"

#include <FreeRTOS.h>

using namespace teller::hal::memory;

namespace teller::hal::memory {

void* malloc(std::size_t size)
{
    /* pvPortMalloc() calls vApplicationMallocFailedHook() for zero-size
     * allocations and we want to avoid that */
    return size > 0 ? pvPortMalloc(size) : nullptr;
}

void free(void* ptr)
{
    return vPortFree(ptr);
}

}
