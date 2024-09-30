#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a heartbeat frame.
 *
 * This structure is not the same as the wire encoding of the heartbeat
 * frame. It contains the raw values of the fields, in SI units where
 * applicable.
 */
typedef struct {
    std::uint32_t timestampInMsec;
    std::uint8_t error;
    obc_mode_t mode;
    float voltageInVolts;
    float temperatureInCelsius;
    struct {
        bool sods;
        bool soe;
        bool lo;
    } rxsmStatusBits;
    struct {
        subsystem_status_t gmm;
        subsystem_status_t scm;
        subsystem_status_t ads;
        subsystem_status_t imu;
        subsystem_status_t mag;
        subsystem_status_t sto;
    } subsystemStatus;
    struct {
        bool gmm;
        bool scm;
        bool suc1;
        bool suc2;
        bool suc3;
        bool cam;
    } lclStatusBits;
} heartbeat_data_t;

/**
 * @brief Encodes a heartbeat frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeHeartbeatFrame(const heartbeat_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a heartbeat frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeHeartbeatFrame(const std::uint8_t* encoded, heartbeat_data_t* decoded);

}
