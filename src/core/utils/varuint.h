#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file varuint.h
 *
 * This file contains routines to store unsigned integers up to 2^30-1 in a
 * variable-length integer encoding. In this encoding, integers are encoded
 * as follows:
 *
 * From 0 to 63, integers are encoded in 1 byte with a bit pattern of \c 00xxxxxx,
 * where the bits marked with \c x store the value itself.
 *
 * From 64 to 2^12-1, integers are encoded in 2 bytes with a bit pattern of
 * \c 010xxxxx 1xxxxxxx.
 *
 * From 2^12 to 2^18-1, integers are encoded in 3 bytes with a bit pattern of
 * \c 0110xxxx 1xxxxxxx 1xxxxxxx.
 *
 * From 2^18 to 2^24-1, integers are encoded in 4 bytes with a bit pattern of
 * \c 01110xxx 1xxxxxxx 1xxxxxxx 1xxxxxxx.
 *
 * From 2^24 to 2^30-1, integers are encoded in 5 bytes with a bit pattern of
 * \c 011110xx 1xxxxxxx 1xxxxxxx 1xxxxxxx 1xxxxxxx.
 *
 * Note the following:
 *
 * - In this encoding, integers always start with a byte whose MSB is 0 and
 *   at least one other bit is set to zero.
 *
 * - Bytes whose MSB is 1 are \em continuation bytes that extend the digits
 *   of the previous byte(s).
 *
 * - When filling the bits marked with \c x from the bits of the value, they
 *   are filled in big endian order (MSB first).
 *
 * - The number of 1s between the most significant zeros in the starting byte
 *   encode the total number of bytes used for the integer minus one.
 *
 * Also note that there exist \em overlong representations for most numbers.
 * For instance, 42 can be encoded as \c 00101010 in its shortest possible form,
 * but a bit sequence of \c 01000000 10101010 is also valid in this encoding and
 * it decodes to \c 000000101010, which is also a binary representation of 42.
 * We call the shortest possible representation of a number its \em compact
 * representation, and we define the k-th \em overlong representation of a
 * number the bit sequence that is exactly k bytes longer than its compact
 * representation. The compact representation can also be considered as the
 * zeroth overlong representation.
 */

/**
 * @brief Incremental varuint decoder that can be fed with bytes iteratively.
 */
typedef struct {
    /** The current varuint being decoded */
    uint32_t value;

    /** Number of bytes left in the current varuint being decoded */
    uint8_t bytes_left;

    /** Number of bytes read for the current varuint being decoded */
    uint8_t bytes_read;
} varuint_decoder_t;

/**
 * @brief Decodes a variable-length encoded unsigned integer from a buffer.
 *
 * @param buf       the buffer containing the value
 * @param value     the decoded value is stored here
 * @return a pointer to right after the last byte that was used for decoding.
 *         When the next byte in the buffer is not a valid starting byte of an
 *         overlong number, the input pointer is returned intact.
 */
const uint8_t* varuint_decode(const uint8_t* buf, uint32_t* value);

/**
 * @brief Decodes a variable-length encoded unsigned integer from a buffer.
 *
 * @param buf       the buffer containing the value
 * @param value     the decoded value is stored here
 * @param overlong  if not NULL, this byte will be set to zero if the decoded
 *        representation was compact or to \em k if the decoded representation
 *        was the k-th overlong representation
 * @return a pointer to right after the last byte that was used for decoding.
 *         When the next byte in the buffer is not a valid starting byte of an
 *         overlong number, the input pointer is returned intact.
 */
const uint8_t* varuint_decode_overlong(const uint8_t* buf, uint32_t* value, uint8_t* overlong);

/**
 * @brief Encodes an unsigned integer in its compact variable-length integer encoding.
 *
 * @param buf       the buffer to store the result into. It must have enough
 *        space to store the representation of the number; use \ref varuint_size
 *        to calculate the space needed for a number
 * @param value     the value to store
 * @return a pointer to right after the last byte of the encoded representation
 */
uint8_t* varuint_encode(uint8_t* buf, uint32_t value);

/**
 * @brief Encodes an unsigned integer in variable-length integer encoding.
 *
 * @param buf       the buffer to store the result into. It must have enough
 *        space to store the representation of the number; use
 *        \ref varuint_size_overlong to calculate the space needed for a number
 * @param value     the value to store
 * @param overlong  set to zero to encode the number in its compact
 *        representation or to \em k to encode its k-th overlong representation
 * @return a pointer to right after the last byte of the encoded representation
 */
uint8_t* varuint_encode_overlong(uint8_t* buf, uint32_t value, uint8_t overlong);

/**
 * @brief Returns the number of bytes needed to store the given unsigned integer
 *        in its compact variable-length integer encoding.
 *
 * @param value     the value to store
 * @return the number of bytes needed to store the given unsigned integer
 */
uint8_t varuint_size(uint32_t value);

/**
 * @brief Returns the number of bytes needed to store the given unsigned integer
 *        in variable-length integer encoding.
 *
 * @param value     the value to store
 * @param overlong  set to zero to return the size of the compact representation
 *        or to \em k to return the size of the k-th overlong representation
 * @return the number of bytes needed to store the given unsigned integer
 */
uint8_t varuint_size_overlong(uint32_t value, uint8_t overlong);

/**
 * @brief Initializes an incremental varuint decoder.
 */
void varuint_decoder_init(varuint_decoder_t* decoder);

/**
 * @brief Destroys an incremental varuint decoder.
 */
void varuint_decoder_destroy(varuint_decoder_t* decoder);

/**
 * @brief Feeds a new byte into an incremental varuint decoder.
 *
 * @param decoder  the decoder to feed
 * @param ch  the byte to feed into the decoder
 * @return Whether a new varuint was decoded successfully
 */
bool varuint_decoder_feed(varuint_decoder_t* decoder, uint8_t ch);

/**
 * @brief Resets an incremental varuint decoder to its base state.
 */
void varuint_decoder_reset(varuint_decoder_t* decoder);

/**
 * @brief Gets the current value from the varuint decoder.
 *
 * This function must be called only right after \ref varuint_decoder_feed()
 * returned true. The returned value is unspecified if this does not hold.
 *
 * @param decoder  the decoder
 */
uint32_t varuint_decoder_get_value(const varuint_decoder_t* decoder);

/**
 * @brief Returns the overlongness of the current varuint that was decoded.
 *
 * This function must be called only right after \ref varuint_decoder_feed()
 * returned true. The returned value is unspecified if this does not hold.
 *
 * @param decoder  the decoder
 * @return zero if the decoded varuint was in its most compact representation,
 * or k if it was the k-th overlong representation
 */
uint8_t varuint_decoder_get_overlong(const varuint_decoder_t* decoder);

#ifdef __cplusplus
}
#endif
