#include "hal/memory.h"

#include <FreeRTOS.h>

using namespace teller::hal::memory;

namespace teller::hal::memory {

void* malloc(std::size_t size)
{
    return pvPortMalloc(size);
}

void free(void* ptr)
{
    return vPortFree(ptr);
}

}
