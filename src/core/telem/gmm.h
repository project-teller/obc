#pragma once

#include "core/math/vector.hpp"
#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a GMM frame.
 *
 * This structure is not the same as the wire encoding of the GMM frame.
 * It contains the raw values of the fields, in SI units where applicable.
 */
typedef struct {
    uint32_t timestampInMsec;
    union {
        uint8_t byIndex[10];
        struct {
            uint8_t c1;
            uint8_t c2;
            uint8_t c3;
            uint8_t c4;
            uint8_t c12;
            uint8_t c13;
            uint8_t c14;
            uint8_t c23;
            uint8_t c24;
            uint8_t c34;
        } byName;
    } hitCounts;
} gmm_data_t;

/**
 * @brief Encodes a GMM frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeGMMFrame(const gmm_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a GMM frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeGMMFrame(const std::uint8_t* encoded, gmm_data_t* decoded);

}
