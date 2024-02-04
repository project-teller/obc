#include "core/telem/timesync.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a timesync frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the timesync frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint64_t rtcTimestamp;
} timesync_frame_t;

uint8_t encodeTimesyncFrame(
    const timesync_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<timesync_frame_t*>(encoded);

    frame->timestamp = data->timestampInMsec;
    frame->rtcTimestamp = data->rtcTimestampInMsec;

    return sizeof(timesync_frame_t);
}

void decodeTimesyncFrame(const uint8_t* encoded, timesync_data_t* decoded)
{
    auto frame = reinterpret_cast<const timesync_frame_t*>(encoded);

    decoded->timestampInMsec = frame->timestamp;
    decoded->rtcTimestampInMsec = frame->rtcTimestamp;
}

}
