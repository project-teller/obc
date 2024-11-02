#pragma once

#include "core/math/vector.hpp"
#include "core/telem/generic.h"

namespace teller::telem::frames {

const int MAX_SCM_FRAME_FRAGMENT_LENGTH = teller::telem::MAX_PAYLOAD_LENGTH - 7;

/**
 * @brief Structure containing all the data required to construct an SCM frame.
 *
 * This structure is not the same as the wire encoding of the SCM frame.
 * It contains the raw values of the fields, in SI units where applicable.
 */
typedef struct {
    uint32_t timestampInMsec;
    uint8_t maxFragmentIndex;
    uint8_t fragmentIndex;
    uint8_t scintillatorIndex;
    uint8_t data[MAX_SCM_FRAME_FRAGMENT_LENGTH];
    uint8_t length;
} scm_data_t;

/**
 * @brief Encodes an SCM frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeSCMFrame(const scm_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes an SCM frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeSCMFrame(const std::uint8_t* encoded, size_t length, scm_data_t* decoded);

/**
 * @brief Validates the wire representation of an SCM frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedSCMFrame(const uint8_t* encoded, size_t length);

}
