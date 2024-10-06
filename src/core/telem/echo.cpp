#include "core/telem/echo.h"

#include <cstring>

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

uint8_t encodeEchoFrame(const echo_data_t* data, uint8_t* encoded)
{
    *encoded = data->isReply;
    memcpy(encoded + 1, data->data, data->data_length);
    return data->data_length + 1;
}

void decodeEchoFrame(const uint8_t* encoded, size_t length, echo_data_t* decoded)
{
    decoded->isReply = *encoded;
    decoded->data_length = length >= 1 ? length - 1 : 0;
    memcpy(decoded->data, encoded + 1, decoded->data_length);
}

bool validateEncodedEchoFrame(const uint8_t* encoded, size_t length)
{
    return length >= 1;
}

}
