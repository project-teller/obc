#pragma once

#include "core/gmm/generic.h"

#include <cstdint>

namespace teller::gmm {

typedef enum {
    WAITING_SYNC_BYTE_START,
    WAITING_SYNC_BYTE_END,
    WAITING_NEWLINE,
    DONE,
} ParserState;

/**
 * @brief Stateful parser to parse NMEA-like messages from a stream of bytes.
 */
class Parser {

public:
    Parser();

    /**
     * @brief Feeds a new byte into the parser.
     *
     * @param   ch  the byte to feed into the parser
     * @return  whether the byte just fed into the parser was the last character
     *          in a message
     */
    bool feed(std::uint8_t ch);

    /**
     * @brief Returns the current message.
     *
     * When feed() returned true, the full message is returned without the
     * trailing newlines. When feed() returned false, a partial message is
     * returned.
     */
    const uint8_t* getMessage() const
    {
        return _message;
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

    /** Buffer in which the current message is being stored */
    uint8_t _message[MAX_MESSAGE_LENGTH + 1];

    uint8_t* _message_write_ptr;

    /** Internal implementation of \c feed() that returns the new state */
    ParserState _feed(std::uint8_t ch);

    /**
     * Stores a single character in the message buffer and advances the write
     * pointer, with bounds checks.
     *
     * @return  whether the character was stored; false if the buffer was full
     */
    [[nodiscard]] bool _store(std::uint8_t ch);
};

}
