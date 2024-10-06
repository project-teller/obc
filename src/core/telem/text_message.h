#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

/** Maximum length of a text message allowed in a telemetry message, inclusive */
const int MAX_TEXT_MESSAGE_LENGTH = MAX_PAYLOAD_LENGTH - 1;

/**
 * @brief Structure containing all the data required to construct a text message frame.
 *
 * This structure is not the same as the wire encoding of the text message
 * frame. It contains the raw values of the fields.
 */
typedef struct {
    /** The module that sends the log message */
    module_id_t module;

    /** The severity level of the message */
    log_level_t level;

    /**
     * The message itself, null-terminated. Note that the maximum allowed
     * message length is therefore \c MAX_PAYLOAD_LENGTH - 1 because one
     * byte is needed to store the module ID and the log level. The buffer
     * has a length of \c MAX_PAYLOAD_LENGTH to allow for the trailing zero
     * in the string, but the trailing zero is not transmitted.
     */
    char message[MAX_PAYLOAD_LENGTH];
} text_message_data_t;

/**
 * @brief Encodes a text message into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeTextMessageFrame(
    const text_message_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a text message frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeTextMessageFrame(
    const std::uint8_t* encoded, std::size_t length, text_message_data_t* decoded);

/**
 * @brief Validates the wire representation of a text message frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedTextMessageFrame(const uint8_t* encoded, size_t length);

}
