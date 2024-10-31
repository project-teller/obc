#pragma once

#include "core/math/vector.hpp"
#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a MAG frame.
 *
 * This structure is not the same as the wire encoding of the MAG frame.
 * It contains the raw values of the fields. The magnetic vector is specified in
 * milligauss.
 */
typedef struct {
    std::uint32_t timestampInMsec;
    teller::math::Vector3f magneticVector;
    float temperature;
} mag_data_t;

/**
 * @brief Encodes a MAG frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeMAGFrame(const mag_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a MAG frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeMAGFrame(const std::uint8_t* encoded, mag_data_t* decoded);

/**
 * @brief Validates the wire representation of a MAG frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedMAGFrame(const uint8_t* encoded, size_t length);

}
