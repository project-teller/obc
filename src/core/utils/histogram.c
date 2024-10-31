#include "core/utils/histogram.h"
#include "core/utils/varuint.h"

size_t histogram_get_packed_size(const uint16_t* histogram, size_t size)
{
    size_t i;
    uint32_t run_length;
    size_t result = 0;

    for (i = 0; i < size; i++) {
        /* Decide whether we will have a value or a run of zeros */
        if (histogram[i] != 0) {
            /* Add the length of the encoded the value */
            result += varuint_size(histogram[i]);
        } else {
            /* Find out the run length */
            run_length = 1;
            while (i + 1 < size && histogram[i + 1] == 0 && run_length < UINT32_MAX) {
                i++;
                run_length++;
            }

            if (run_length > 1) {
                /* Add the length of the first overlong representation of the run length */
                result += varuint_size_overlong(run_length, 1);
            } else {
                /* Just encode the zero, it's shorter */
                result += varuint_size(0);
            }
        }
    }

    return result;
}

size_t histogram_get_unpacked_size(const uint8_t* buf, size_t size)
{
    const uint8_t *read_ptr = buf, *end = buf + size;
    size_t result = 0;
    uint32_t x;
    uint8_t overlong;

    while (read_ptr < end) {
        read_ptr = varuint_decode_overlong(read_ptr, &x, &overlong);
        if (overlong) {
            /* This number encodes the length of a run of zeros */
            result += x;
        } else {
            /* This number encodes the value of a single histogram cell */
            result++;
        }
    }

    /* Return zero if the input seems invalid as we have read past the end of
     * the buffer */
    return read_ptr == end ? result : 0;
}

uint8_t* histogram_pack(uint8_t* buf, const uint16_t* histogram, size_t size)
{
    size_t i;
    uint32_t run_length;
    uint8_t* write_ptr = buf;

    for (i = 0; i < size; i++) {
        /* Decide whether we will have a value or a run of zeros */
        if (histogram[i] != 0) {
            /* Encode the value */
            write_ptr = varuint_encode(write_ptr, histogram[i]);
        } else {
            /* Find out the run length */
            run_length = 1;
            while (i + 1 < size && histogram[i + 1] == 0 && run_length < UINT32_MAX) {
                i++;
                run_length++;
            }

            if (run_length > 1) {
                /* Encode the run length with its first overlong representation */
                write_ptr = varuint_encode_overlong(write_ptr, run_length, 1);
            } else {
                /* Just encode the zero, it's shorter */
                write_ptr = varuint_encode(write_ptr, 0);
            }
        }
    }

    return write_ptr;
}

uint16_t* histogram_unpack(const uint8_t* buf, uint16_t* value, size_t size)
{
    const uint8_t *read_ptr = buf, *next_read_ptr, *end = buf + size;
    uint32_t x;
    uint8_t overlong;

    while (read_ptr < end) {
        next_read_ptr = varuint_decode_overlong(read_ptr, &x, &overlong);
        if (next_read_ptr <= read_ptr) {
            /* Error while decoding */
            break;
        }
        read_ptr = next_read_ptr;

        if (overlong) {
            /* This number encodes the length of a run of zeros */
            while (x > 0) {
                *value = 0;
                value++;
                x--;
            }
        } else if (x <= UINT16_MAX) {
            /* This number encodes the value of a single histogram cell */
            *value = x;
            value++;
        } else {
            /* Hmmm, overflow */
            *value = 0;
            value++;
        }
    }

    return value;
}
