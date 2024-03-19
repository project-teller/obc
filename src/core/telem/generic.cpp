#include <cstring>

#include "core/telem/generic.h"
#include "core/utils/crc.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem {

uint8_t getMessageSizeForPayloadLength(uint8_t payload_length)
{
    return payload_length <= MAX_PAYLOAD_LENGTH ? payload_length + 8 : 0;
}

uint8_t serialize(
    uint8_t* buffer, uint8_t buffer_length,
    envelope_t envelope, const uint8_t* payload, uint8_t payload_length)
{
    uint8_t space_needed = getMessageSizeForPayloadLength(payload_length);
    uint16_t crc;

    if (space_needed == 0 || space_needed > buffer_length) {
        return 0;
    }

    if (envelope.source == UNKNOWN_COMPONENT) {
        envelope.source = ONBOARD_COMPUTER;
    }

    if (envelope.target == UNKNOWN_COMPONENT) {
        envelope.target = GROUND_STATION;
    }

    buffer[0] = 0xCA;
    buffer[1] = 0xFE;
    buffer[2] = envelope.seq_no;
    buffer[3] = envelope.frame_type;
    buffer[4] = (((static_cast<int>(envelope.source) & 0x03) << 4) | (static_cast<int>(envelope.target) & 0x03));
    buffer[5] = payload_length;

    if (payload && payload_length > 0) {
        memcpy(buffer + 6, payload, payload_length);
    }

    crc = crc_ccitt(0, buffer, payload_length + 6);
    buffer[payload_length + 6] = crc & 0xff;
    buffer[payload_length + 7] = crc >> 8;

    return space_needed;
}

}
