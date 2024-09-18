#pragma once

#include <cstdint>

#include "core/telem/generic.h"

namespace teller::telem {

typedef enum {
    WAITING_SYNC_BYTE_1,
    WAITING_SYNC_BYTE_2,
    READING_HEADER,
    READING_PAYLOAD,
    READING_CHECKSUM,
    DONE,
} ParserState;

/**
 * @brief Stateful parser to parse telemetry messages from a stream of bytes.
 */
class Parser {

public:
    Parser();

    /**
     * @brief Feeds a new byte into the parser.
     *
     * @param   ch  the byte to feed into the parser
     * @return  the total length of the payload of the message plus one if the
     *          byte just fed into the parser was the last character in a
     *          telemetry message, zero otherwise
     */
    std::uint8_t feed(std::uint8_t ch);

    /**
     * @brief Returns the envelope of the current message being parsed.
     */
    const envelope_t& getEnvelope() const
    {
        return _envelope;
    }

    /**
     * @brief Returns the payload of the current message being parsed.
     */
    const uint8_t* getPayload() const
    {
        return _message + HEADER_LENGTH;
    }

    /**
     * @brief Returns the current parser state. Only for testing purposes.
     */
    ParserState getState() const
    {
        return _state;
    }

    /**
     * @brief Resets the parser to a base state.
     */
    void reset();

private:
    /** Current state of the parser */
    ParserState _state;

    /** Envelope of the current message being parsed */
    envelope_t _envelope;

    /** Copy of the current message being parsed */
    uint8_t _message[MAX_MESSAGE_LENGTH + 1];

    uint8_t* _message_write_ptr;
    uint8_t* _end;

    /** Internal implementation of \c feed() that returns the new state */
    ParserState _feed(std::uint8_t ch);

    void _updateEnvelope();
};

}
