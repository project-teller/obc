#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

typedef enum {
    CALIBRATION_NOP,
    CALIBRATION_GYRO,
    CALIBRATION_ACCEL,
    NUM_CALIBRATION_PROCEDURES,
} calibration_procedure_t;

/**
 * @brief Structure containing all the data required to construct a request
 * to calibrate some component of the system.
 */
typedef struct {
    /** The calibration procedure to perform */
    calibration_procedure_t procedure;
} calibration_request_data_t;

/**
 * @brief Encodes a calibration request frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeCalibrationRequestFrame(const calibration_request_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a calibration request frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeCalibrationRequestFrame(const std::uint8_t* encoded, calibration_request_data_t* decoded);

}
