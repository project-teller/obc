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
    uint32_t missionClockTimestamp;
    uint64_t rtcTimestamp;
    uint32_t liftoffTimestamp;
} clock_status_frame_t;

uint8_t encodeClockStatusFrame(
    const clock_status_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<clock_status_frame_t*>(encoded);

    frame->timestamp = data->timestampInMsec;
    frame->missionClockTimestamp = data->missionClockInMsec;
    frame->rtcTimestamp = data->rtcTimestampInMsec;
    frame->liftoffTimestamp = data->liftoffTimestampInMsec;

    if (data->missionClockIsRunning) {
        frame->missionClockTimestamp |= 0x80000000U;
    } else {
        frame->missionClockTimestamp &= ~0x80000000U;
    }

    if (data->liftoffHappened) {
        frame->liftoffTimestamp |= 0x80000000U;
    } else {
        frame->liftoffTimestamp &= ~0x80000000U;
    }

    return sizeof(clock_status_frame_t);
}

void decodeClockStatusFrame(const uint8_t* encoded, clock_status_data_t* decoded)
{
    auto frame = reinterpret_cast<const clock_status_frame_t*>(encoded);

    decoded->timestampInMsec = frame->timestamp;
    decoded->rtcTimestampInMsec = frame->rtcTimestamp;

    decoded->missionClockIsRunning = frame->missionClockTimestamp & 0x80000000U;
    decoded->missionClockInMsec = frame->missionClockTimestamp & 0x7FFFFFFFU;

    decoded->liftoffHappened = frame->liftoffTimestamp & 0x80000000U;
    decoded->liftoffTimestampInMsec = frame->liftoffTimestamp & 0x7FFFFFFFU;
}

bool validateEncodedClockStatusFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(clock_status_frame_t);
}

}
