#include "core/telem/clock_sync.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a clock sync frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the clock sync frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint64_t timestamp;
} clock_sync_frame_t;

uint8_t encodeClockSyncFrame(
    const clock_sync_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<clock_sync_frame_t*>(encoded);

    frame->timestamp = data->timestampInMsec;

    return sizeof(clock_sync_frame_t);
}

void decodeClockSyncFrame(const uint8_t* encoded, clock_sync_data_t* decoded)
{
    auto frame = reinterpret_cast<const clock_sync_frame_t*>(encoded);

    decoded->timestampInMsec = frame->timestamp;
}

bool validateEncodedClockSyncFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(clock_sync_frame_t);
}

}
