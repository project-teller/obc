#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Enum containing the possible response codes in an acknowledgment frame.
 */
typedef enum {
    /** Command is accepted and was executed. */
    ACK_ACCEPTED = 0,

    /** Command is invalid because of invalid parameters. Retrying the command
     * with the same parameters will not work. */
    NAK_DENIED = 1,

    /** Command execution was attempted but the command failed. Retrying the
     * command might work. */
    NAK_FAILED = 2,

    /** Command is not supported. */
    NAK_UNSUPPORTED = 3,

    /** Invalid response; used only when deserializing a frame with an unknown
     * response code. */
    NAK_INVALID,

    NUM_ACK_RESULT_CODES,
} ack_result_t;

/**
 * @brief Structure containing all the data required to construct an
 * acknowledgment frame.
 */
typedef struct {
    /** Type of the frame that the acknowledgment refers to */
    frame_type_t frame_type;

    /** Sequence number of the frame */
    uint8_t seq_no;

    /** Result of the command that was executed */
    ack_result_t result;

    /**
     * Optional POSIX-style error code for negative acknowledgments. Used only
     * when the acknowledgment result is not ACK_ACCEPTED.
     */
    int error;

    /**
     * Optional 32-bit value to return for positive acknowledgments. Used only
     * when the acknowledgment result is ACK_ACCEPTED.
     */
    uint32_t value;
} ack_data_t;

/**
 * @brief Encodes an acknowledgment frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeAckFrame(const ack_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes an acknowledgment frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeAckFrame(const std::uint8_t* encoded, ack_data_t* decoded);

/**
 * @brief Validates the wire representation of an acknowledgment frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedAckFrame(const uint8_t* encoded, size_t length);

}
