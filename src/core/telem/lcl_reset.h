#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

typedef enum {
    LCL_RESET_GMM = 1,
    LCL_RESET_SCM = 2,
    LCL_RESET_SUC1 = 4,
    LCL_RESET_SUC2 = 8,
    LCL_RESET_SUC3 = 16,
    LCL_RESET_CAM = 32,
    LCL_RESET_ALL = 63,
} lcl_reset_bit_t;

/**
 * @brief Structure containing all the data required to construct a request for
 * the reset of one or more latching current limiters.
 */
typedef struct {
    uint8_t lcls_to_reset;
} lcl_reset_request_data_t;

/**
 * @brief Encodes a LCL reset request frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeLCLResetRequestFrame(const lcl_reset_request_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a LCL reset request frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeLCLResetRequestFrame(const std::uint8_t* encoded, lcl_reset_request_data_t* decoded);

/**
 * @brief Validates the wire representation of a LCL reset request frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedLCLResetRequestFrame(const uint8_t* encoded, size_t length);

}
