#include <math.h>
#include <stdlib.h>

#include "core/utils/random.h"

#define M_PI_F 3.14159265358979323846f

float rng_gauss()
{
    float x = (float)random() / RAND_MAX,
          y = (float)random() / RAND_MAX,
          z = sqrtf(-2 * logf(x)) * cosf(2 * M_PI_F * y);
    return z;
}
