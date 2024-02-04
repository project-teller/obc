#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a timesync frame.
 */
typedef struct {
    std::uint32_t timestampInMsec;
    std::uint64_t rtcTimestampInMsec;
} timesync_data_t;

/**
 * @brief Encodes a timesync frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeTimesyncFrame(const timesync_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a timesync frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeTimesyncFrame(const std::uint8_t* encoded, timesync_data_t* decoded);

}
