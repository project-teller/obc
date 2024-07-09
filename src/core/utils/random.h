#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generates a random number according to the standard normal distribution.
 */
float rng_gauss();

/**
 * @brief Generates a random number uniformly distributed in [0; 1)
 */
float rng_unif01();

#ifdef __cplusplus
}
#endif
