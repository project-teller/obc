#include <cassert>
#include <cstring>

#include "core/telem/text_message.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

uint8_t encodeTextMessageFrame(
    const text_message_data_t* data, uint8_t* encoded)
{
    size_t length = strlen(data->message);

    assert(length <= MAX_PAYLOAD_LENGTH - 1);

    encoded[0] = (data->level & 0x07) | (data->module << 3);
    memcpy(encoded + 1, data->message, length);

    return length + 1;
}

void decodeTextMessageFrame(
    const uint8_t* encoded, size_t length, text_message_data_t* decoded)
{
    assert(length <= MAX_PAYLOAD_LENGTH);

    decoded->level = static_cast<log_level_t>(encoded[0] & 0x07);
    decoded->module = static_cast<module_id_t>(encoded[0] >> 3);

    memcpy(decoded->message, encoded + 1, length - 1);
    decoded->message[length - 1] = 0;
}

}
