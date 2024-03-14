#include "core/telem/ack.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of an acknowledgment frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the acknowledgment frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t frame_type;
    uint8_t seq_no;
    uint8_t result;
    int16_t error;
} ack_frame_t;

uint8_t encodeAckFrame(const ack_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<ack_frame_t*>(encoded);
    uint8_t code = static_cast<uint8_t>(data->result);

    if (code < ACK_ACCEPTED || code >= NAK_INVALID) {
        code = NAK_INVALID;
    }

    frame->frame_type = data->frame_type;
    frame->seq_no = data->seq_no;
    frame->result = code;
    frame->error = data->error;

    return sizeof(ack_frame_t);
}

void decodeAckFrame(const uint8_t* encoded, ack_data_t* decoded)
{
    auto frame = reinterpret_cast<const ack_frame_t*>(encoded);
    uint8_t code = static_cast<uint8_t>(frame->result);

    if (code < ACK_ACCEPTED || code >= NAK_INVALID) {
        code = NAK_INVALID;
    }

    decoded->frame_type = static_cast<frame_type_t>(frame->frame_type);
    decoded->seq_no = frame->seq_no;
    decoded->result = static_cast<ack_result_t>(code);
    decoded->error = frame->error;
}

}
