#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file histogram_encoding.h
 *
 * This file contains routines to compactly encode histograms using a varuint
 * encoding, under the assumption that most of the histogram bins are empty and
 * that the remaining values are Poisson-distributed.
 *
 * In this encoding, histogram bins are encoded sequentially such that non-empty
 * bins are represented by the compact varuint representation of the number of
 * items in the bin, \em except that sequences of more than 2 empty bins are
 * represented by the first \em overlong representation of the number of empty
 * bins in the sequence.
 *
 * The maximum item count in each bin of the histogram is 65536.
 */

/**
 * @brief Returns the number of bytes that would be needed to store a packed histogram.
 *
 * @param histogram the histogram to store
 * @param size      the number of bins in the histogram
 * @return the number of bytes that would be needed to store the histogram
 */
size_t histogram_get_packed_size(const uint16_t* histogram, size_t size);

/**
 * @brief Returns the number of bytes that would be needed to store an unpacked histogram.
 *
 * @param buf   the buffer containing the packed histogram
 * @param size  the length of the buffer
 * @return the number of bytes that would be needed to store the histogram
 */
size_t histogram_get_unpacked_size(const uint8_t* buf, size_t size);

/**
 * @brief Packs a histogram into a buffer.
 *
 * @param buf       the buffer to store the result into. It must have enough
 *        space to store the representation of the histogram; use \ref histogram_get_packed_size
 *        to calculate the space needed for a histogram
 * @param histogram the histogram to store
 * @param size      the number of bins in the histogram
 * @return a pointer to right after the last byte of the packed representation
 */
uint8_t* histogram_pack(uint8_t* buf, const uint16_t* histogram, size_t size);

/**
 * @brief Unpacks a packed histogram from a buffer.
 *
 * @param buf       the buffer containing the encoded histogram
 * @param value     the decoded histogram is stored here.
 * @param size  the length of the buffer
 * @return a pointer to right after the last byte of the unpacked representation
 */
uint16_t* histogram_unpack(const uint8_t* buf, uint16_t* value, size_t size);

#ifdef __cplusplus
}
#endif
