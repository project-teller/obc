#include "hal/memory.h"

using namespace teller::hal::memory;

namespace teller::hal::memory {

void* malloc(std::size_t size)
{
    return std::malloc(size);
}

void free(void* ptr)
{
    return std::free(ptr);
}

}
