#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct an IMU frame.
 *
 * This structure is not the same as the wire encoding of the IMU frame.
 * It contains the raw values of the fields, in SI units where applicable.
 */
typedef struct {
    std::uint32_t timestampInMsec;
    struct {
        float x;
        float y;
        float z;
    } acceleration;
    struct {
        float x;
        float y;
        float z;
    } angularVelocity;
} imu_data_t;

/**
 * @brief Encodes an IMU frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeIMUFrame(const imu_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes an IMU frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeIMUFrame(const std::uint8_t* encoded, imu_data_t* decoded);

}
