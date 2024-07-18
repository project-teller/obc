#pragma once

#include "core/telem/generic.h"

namespace teller::telem {

const int MAX_BINARY_DATA_FRAGMENT_LENGTH = MAX_PAYLOAD_LENGTH - 4;

};

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
 * @brief Structure containing all the data required to construct a binary
 * data frame.
 */
typedef struct {
    /** Type of the frame that the binary data packet responds to */
    frame_type_t frame_type;

    /** Sequence number of the frame that the binary data packet responds to */
    uint8_t seq_no;

    /** Maximum fragment index in the batch being sent */
    uint8_t max_fragment_index;

    /** Current fragment index in the batch being sent */
    uint8_t fragment_index;

    /** Data to send in the frame */
    uint8_t data[MAX_BINARY_DATA_FRAGMENT_LENGTH];

    /** Number of bytes actually used from the data array */
    uint8_t data_length;
} binary_data_t;

/**
 * @brief Encodes a binary data frame into its wire representation.
 *
 * @param data  the data to encode in the binary data frame
 * @param encoded  the encoded representation of the binary data frame
 * @return the length of the encoded representation of the binary data frame
 */
uint8_t encodeBinaryDataFrame(const binary_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a binary data frame from its wire representation.
 *
 * @param encoded  the encoded representation of the binary data frame
 * @param decoded  the decoded representation of the binary data ame
 */
void decodeBinaryDataFrame(const std::uint8_t* encoded, size_t length, binary_data_t* decoded);

}
