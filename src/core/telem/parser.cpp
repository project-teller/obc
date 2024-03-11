#include "core/telem/parser.h"
#include "core/utils/crc.h"

#include <cstring>

using namespace std;

const uint8_t SYNC_WORD_BYTES[2] = { 0xCA, 0xFE };

namespace teller::telem {

Parser::Parser()
{
    reset();
}

void Parser::reset()
{
    _state = WAITING_SYNC_BYTE_1;
    _end = _message;
    _message_write_ptr = _message;
    memset(&_envelope, 0, sizeof(_envelope));
}

bool Parser::feed(std::uint8_t ch)
{
    _state = _feed(ch);

    if (_state == DONE) {
        reset();
        _updateEnvelope();
        return true;
    } else {
        return false;
    }
}

ParserState Parser::_feed(std::uint8_t ch)
{
    switch (_state) {

    /* WAITING_SYNC_BYTE_1 intentionally skipped, this is the same as
     * the catch-all case */
    case WAITING_SYNC_BYTE_2:
        if (ch == SYNC_WORD_BYTES[1]) {
            *(_message_write_ptr++) = ch;
            _end = _message_write_ptr + 4;
            return READING_HEADER;
        } else {
            return WAITING_SYNC_BYTE_1;
        }

    case READING_HEADER:
        *(_message_write_ptr++) = ch;
        if (_message_write_ptr == _end) {
            if (ch <= MAX_PAYLOAD_LENGTH) {
                _end += ch;
                return READING_PAYLOAD;
            } else {
                reset();
                return WAITING_SYNC_BYTE_1;
            }
        } else {
            return READING_HEADER;
        }
        break;

    case READING_PAYLOAD:
        *(_message_write_ptr++) = ch;
        if (_message_write_ptr == _end) {
            _end += 2;
            return READING_CHECKSUM;
        } else {
            return READING_PAYLOAD;
        }
        break;

    case READING_CHECKSUM:
        *(_message_write_ptr++) = ch;
        if (_message_write_ptr == _end) {
            uint16_t crc = crc_ccitt(0, _message, _end - _message);
            if (crc == 0) {
                return DONE;
            } else {
                reset();
                return WAITING_SYNC_BYTE_1;
            }
        } else {
            return READING_CHECKSUM;
        }
        break;

    default:
        if (ch == SYNC_WORD_BYTES[0]) {
            reset();
            *(_message_write_ptr++) = ch;
            return WAITING_SYNC_BYTE_2;
        } else {
            return WAITING_SYNC_BYTE_1;
        }
    }
}

void Parser::_updateEnvelope()
{
    _envelope.seq_no = _message[2];
    _envelope.frame_type = _message[3];
    _envelope.source = static_cast<component_t>(_message[4] >> 4);
    _envelope.target = static_cast<component_t>(_message[4] & 0x0F);
}

}
