#pragma once

#include <cstdlib>

namespace teller::hal::memory {

/**
 * @brief Allocates a chunk of memory with the given size.
 */
void* malloc(std::size_t size);

/**
 * @brief Frees a chunk of memory previously allocated with malloc()
 */
void free(void* ptr);

}
