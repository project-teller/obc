#include "core/telem/binary_data.h"

#include <cstring>

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a binary data frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the binary data frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t frame_type;
    uint8_t seq_no;
    uint8_t max_fragment_index;
    uint8_t fragment_index;
    uint8_t data[MAX_BINARY_DATA_FRAGMENT_LENGTH];
} binary_data_frame_t;

uint8_t encodeBinaryDataFrame(const binary_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<binary_data_frame_t*>(encoded);

    frame->frame_type = data->frame_type;
    frame->seq_no = data->seq_no;
    frame->max_fragment_index = data->max_fragment_index;
    frame->fragment_index = data->fragment_index;

    memcpy(frame->data, data->data, data->data_length);

    return data->data_length + 4;
}

void decodeBinaryDataFrame(const uint8_t* encoded, size_t length, binary_data_t* decoded)
{
    auto frame = reinterpret_cast<const binary_data_frame_t*>(encoded);

    decoded->frame_type = static_cast<frame_type_t>(frame->frame_type);
    decoded->seq_no = frame->seq_no;
    decoded->max_fragment_index = frame->max_fragment_index;
    decoded->fragment_index = frame->fragment_index;

    decoded->data_length = length >= 4 ? length - 4 : 0;
    memcpy(decoded->data, frame->data, decoded->data_length);
}

}
