#include <stdlib.h>

#include "core/utils/histogram.h"
#include "core/utils/varuint.h"

size_t histogram_get_packed_size(const uint16_t* histogram, size_t size)
{
    /* TODO(ntamas) */
    return 0;
}

size_t histogram_get_unpacked_size(const uint8_t* buf, size_t size)
{
    /* TODO(ntamas) */
    return 0;
}

uint8_t* histogram_pack(uint8_t* buf, const uint16_t* histogram, size_t size)
{
    /* TODO(ntamas) */
    return buf;
}

uint16_t* histogram_unpack(const uint8_t* buf, uint16_t* value, size_t size)
{
    /* TODO(ntamas) */
    return value;
}
