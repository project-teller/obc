#include <stdlib.h>

#include "core/utils/varuint.h"

/**
 * @def MAX_LENGTH
 * @brief Maximum length of a variable-length integer, in bytes.
 */
#define MAX_LENGTH 5

typedef struct {
    uint8_t mask;
    uint8_t pattern;
} uint8_pattern_t;

/**
 * @brief Marker prefixes for variable-length integers with the given number of
 * total byte length.
 */
static const uint8_pattern_t MARKERS[] = {
    { .mask = 0b11000000, .pattern = 0b00000000 },
    { .mask = 0b11100000, .pattern = 0b01000000 },
    { .mask = 0b11110000, .pattern = 0b01100000 },
    { .mask = 0b11111000, .pattern = 0b01110000 },
    { .mask = 0b11111100, .pattern = 0b01111000 },
    { .mask = 0b11111110, .pattern = 0b01111100 },
    { .mask = 0b11111111, .pattern = 0b01111110 },
    { .mask = 0b11111111, .pattern = 0b01111111 },
    { .mask = 0, .pattern = 0 } /* sentinel */
};

static const uint32_t THRESHOLDS[MAX_LENGTH] = {
    1 << 6, 1 << 12, 1 << 18, 1 << 24, 1 << 30
};

const uint8_t* varuint_decode(const uint8_t* buf, uint32_t* value)
{
    return varuint_decode_overlong(buf, value, NULL);
}

const uint8_t* varuint_decode_overlong(const uint8_t* buf, uint32_t* value, uint8_t* overlong)
{
    int i, size;
    uint32_t result;

    if (buf[0] & 0x80) {
        /* MSB set, this cannot be the start of a varuint */
        goto end;
    }

    if ((buf[0] & 0xC0) == 0) {
        /* Shortcut: one-byte non-overlong representation */
        if (value) {
            *value = buf[0];
        }
        if (overlong) {
            *overlong = 0;
        }
        buf++;
        goto end;
    }

    /* Determine the length of the varuint in bytes */
    for (i = 0; (buf[0] & MARKERS[i].mask) != MARKERS[i].pattern; i++)
        ;
    if (!MARKERS[i].mask) {
        /* No match for markers, this cannot be the start of a varuint,
         * but this should not happen either */
        /* LCOV_EXCL_START */
        goto end;
        /* LCOV_EXCL_STOP */
    }

    /* Start assembling the varuint */
    result = buf[0] & ~MARKERS[i].mask;
    size = i + 1;
    for (i = 1; i < size; i++) {
        if ((buf[i] & 0x80) != 0x80) {
            /* MSB not set, invalid length */
            goto end;
        }

        result <<= 7;
        result |= buf[i] & 0x7F;
    }

    if (value) {
        *value = result;
    }
    if (overlong) {
        *overlong = size - varuint_size(result);
    }
    buf += size;

end:
    return buf;
}

uint8_t* varuint_encode(uint8_t* buf, uint32_t value)
{
    return varuint_encode_overlong(buf, value, 0);
}

uint8_t* varuint_encode_overlong(uint8_t* buf, uint32_t value, uint8_t overlong)
{
    int i, size;

    size = varuint_size_overlong(value, overlong);
    if (size == 1) {
        /* Shortcut: representation is the same as the value */
        buf[0] = value & 0x3F;
    } else {
        for (i = size - 1; i >= 0; i--, value >>= 7) {
            buf[i] = 0x80 | (value & 0x7F);
        }
        buf[0] = (buf[0] & ~MARKERS[size - 1].mask) | MARKERS[size - 1].pattern;
    }

    return buf + size;
}

uint8_t varuint_size(uint32_t value)
{
    return varuint_size_overlong(value, 0);
}

uint8_t varuint_size_overlong(uint32_t value, uint8_t overlong)
{
    for (uint8_t i = 0; i < MAX_LENGTH; i++) {
        if (value < THRESHOLDS[i]) {
            return i + overlong + 1;
        }
    }

    return MAX_LENGTH + overlong + 1;
}
