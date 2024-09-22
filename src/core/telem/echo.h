#pragma once

#include "core/telem/generic.h"

namespace teller::telem {

const int MAX_ECHO_DATA_LENGTH = MAX_PAYLOAD_LENGTH - 1;

};

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct an echo frame.
 */
typedef struct {
    /** Stores whether this is a request or a reply */
    uint8_t is_reply;

    /** Data to send in the frame */
    uint8_t data[MAX_PAYLOAD_LENGTH];

    /** Number of bytes actually used from the data array */
    uint8_t data_length;
} echo_data_t;

/**
 * @brief Encodes an echo packet into its wire representation.
 *
 * @param data  the data to encode in the echo frame
 * @param encoded  the encoded representation of the echo frame
 * @return the length of the encoded representation of the echo frame
 */
uint8_t encodeEchoFrame(const echo_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes an echo frame from its wire representation.
 *
 * @param encoded  the encoded representation of the echo frame
 * @param decoded  the decoded representation of the echo frame
 */
void decodeEchoFrame(const std::uint8_t* encoded, size_t length, echo_data_t* decoded);

}
