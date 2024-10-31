#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a clock status frame.
 */
typedef struct {
    std::uint32_t timestampInMsec;
    std::uint32_t missionClockInMsec;
    std::uint64_t rtcTimestampInMsec;
    bool missionClockIsRunning;
} clock_status_data_t;

/**
 * @brief Encodes a clock status frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeClockStatusFrame(const clock_status_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a clock status frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeClockStatusFrame(const std::uint8_t* encoded, clock_status_data_t* decoded);

/**
 * @brief Validates the wire representation of a clock status frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedClockStatusFrame(const uint8_t* encoded, size_t length);

}
