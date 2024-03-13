#include "core/telem/clock_status.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a clock status frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the clock status frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint64_t rtcTimestamp;
} clock_status_frame_t;

uint8_t encodeClockStatusFrame(
    const clock_status_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<clock_status_frame_t*>(encoded);

    frame->timestamp = data->timestampInMsec;
    frame->rtcTimestamp = data->rtcTimestampInMsec;

    return sizeof(clock_status_frame_t);
}

void decodeClockStatusFrame(const uint8_t* encoded, clock_status_data_t* decoded)
{
    auto frame = reinterpret_cast<const clock_status_frame_t*>(encoded);

    decoded->timestampInMsec = frame->timestamp;
    decoded->rtcTimestampInMsec = frame->rtcTimestamp;
}

}
