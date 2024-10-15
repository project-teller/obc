#include "core/telem/lcl_reset.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a LCL reset request in the telemetry protocol.
 *
 * This structure represents the wire encoding of the LCL reset request frame.
 * There is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t lcls_to_reset;
} lcl_reset_request_frame_t;

uint8_t encodeLCLResetRequestFrame(
    const lcl_reset_request_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<lcl_reset_request_frame_t*>(encoded);
    frame->lcls_to_reset = data->lcls_to_reset & LCL_RESET_ALL;

    return sizeof(lcl_reset_request_frame_t);
}

void decodeLCLResetRequestFrame(const uint8_t* encoded, lcl_reset_request_data_t* decoded)
{
    auto frame = reinterpret_cast<const lcl_reset_request_frame_t*>(encoded);
    decoded->lcls_to_reset = frame->lcls_to_reset & LCL_RESET_ALL;
}

bool validateEncodedLCLResetRequestFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(lcl_reset_request_frame_t);
}

}
