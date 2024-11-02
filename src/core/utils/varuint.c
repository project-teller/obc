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
    1UL << 6, 1UL << 12, 1UL << 18, 1UL << 24, 1UL << 30
};

const uint8_t* varuint_decode(const uint8_t* buf, uint32_t* value)
{
    return varuint_decode_overlong(buf, value, 0);
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

void varuint_decoder_init(varuint_decoder_t* decoder)
{
    varuint_decoder_reset(decoder);
}

void varuint_decoder_destroy(varuint_decoder_t* decoder)
{
    varuint_decoder_reset(decoder);
}

void varuint_decoder_reset(varuint_decoder_t* decoder)
{
    decoder->bytes_left = 0;
    decoder->bytes_read = 0;
    decoder->value = 0;
}

bool varuint_decoder_feed(varuint_decoder_t* decoder, uint8_t ch)
{
    int i;

    if ((ch & 0x80) == 0) {
        /* This byte is the start of a new varuint */
        for (i = 0; (ch & MARKERS[i].mask) != MARKERS[i].pattern; i++)
            ;
        if (!MARKERS[i].mask) {
            /* No match for markers, this cannot be the start of a varuint,
             * but this should not happen either */
            /* LCOV_EXCL_START */
            return false;
            /* LCOV_EXCL_STOP */
        }

        decoder->bytes_left = i;
        decoder->bytes_read = 1;
        decoder->value = ch & ~MARKERS[i].mask;
    } else if (decoder->bytes_left > 0) {
        /* This byte is the continuation of the varuint */
        decoder->bytes_left--;
        decoder->bytes_read++;
        decoder->value = (decoder->value << 7) | (ch & 0x7F);
    } else {
        /* This byte is supposed to be a continuation, but we are not expecting
         * more bytes for the varuint */
        return false;
    }

    return decoder->bytes_left == 0;
}

uint32_t varuint_decoder_get_value(const varuint_decoder_t* decoder)
{
    return decoder->value;
}

uint8_t varuint_decoder_get_overlong(const varuint_decoder_t* decoder)
{
    return decoder->bytes_read - varuint_size(decoder->value);
}
