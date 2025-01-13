#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

typedef enum {
    DEBUG_CMD_NOP,
    DEBUG_CMD_START_CLOCK,
    DEBUG_CMD_STOP_CLOCK,
    DEBUG_CMD_RESET_CLOCK,
    DEBUG_CMD_TOGGLE_CAMERA,
    DEBUG_CMD_TRIGGER_WATCHDOG,
    DEBUG_CMD_REPORT_STORAGE_STATUS,
    DEBUG_CMD_TOGGLE_SIMULATED_ERROR,
    DEBUG_CMD_REPORT_EDR_STATUS,
    DEBUG_CMD_TOGGLE_TELEMETRY_LEVEL,
    DEBUG_CMD_REPORT_STACK_USAGE,
    NUM_DEBUG_COMMANDS,
} debug_command_t;

/**
 * @brief Structure containing all the data required to construct a request
 * to execute a debug command.
 */
typedef struct {
    /** The debug command to execute */
    debug_command_t command;
} debug_command_data_t;

/**
 * @brief Encodes a debug command frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeDebugCommandFrame(const debug_command_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a debug command from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeDebugCommandFrame(const std::uint8_t* encoded, debug_command_data_t* decoded);

/**
 * @brief Validates the wire representation of a debug command without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedDebugCommandFrame(const uint8_t* encoded, size_t length);

}
