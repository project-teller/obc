#include "core/nmea/parser.h"

#include <cstring>
#include <minmea.h>

using namespace std;

const uint8_t SYNC_BYTE_START = '$';
const uint8_t SYNC_BYTE_END = '*';

namespace teller::nmea {

Parser::Parser()
{
    std::memset(_message, 0, sizeof(_message));
    reset();
}

void Parser::reset()
{
    _state = WAITING_SYNC_BYTE_START;
    _message_write_ptr = _message;

    /* Do not clear _message at this point, otherwise getMessage() would not
     * work after successfully parsing a message */
}

bool Parser::feed(std::uint8_t ch)
{
    _state = _feed(ch);

    if (_state == ERROR) {
        reset();
        return false;
    } else if (_state == DONE) {
        reset();
        return minmea_check(getMessage(), /* strict = */ true);
    } else {
        return false;
    }
}

ParserState Parser::_feed(std::uint8_t ch)
{
    ParserState nextState = _state;
    bool toStore = false;

    if (!isprint(ch) && ch != '\n' && ch != '\r') {
        return ERROR;
    } else if (ch == SYNC_BYTE_START) {
        reset();
        toStore = true;
        nextState = WAITING_SYNC_BYTE_END;
    } else {
        /* WAITING_SYNC_BYTE_1 intentionally skipped, this is the same as
         * the catch-all case */

        switch (_state) {
        case WAITING_SYNC_BYTE_END:
            toStore = true;
            if (ch == SYNC_BYTE_END) {
                nextState = WAITING_NEWLINE;
            }
            break;

        case WAITING_NEWLINE:
            if (ch == '\r' || ch == '\n') {
                nextState = DONE;
            } else {
                toStore = true;
            }
            break;

        default:
            nextState = WAITING_SYNC_BYTE_START;
        }
    }

    if (toStore && !_store(ch)) {
        /* Message too long */
        nextState = WAITING_SYNC_BYTE_START;
    }

    return nextState;
}

bool Parser::_store(std::uint8_t ch)
{
    if (_message_write_ptr <= _message + MAX_MESSAGE_LENGTH) {
        *(_message_write_ptr++) = ch;
        *(_message_write_ptr) = 0; /* OK because the buffer is one larger */
        return true;
    } else {
        return false;
    }
}

}
