#include <cstring>

#include "core/telem/gmm.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a GMM frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the GMM frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint8_t counts[10];
} gmm_frame_t;

static_assert(sizeof(gmm_frame_t) == 14, "GMM frame size invalid");

uint8_t encodeGMMFrame(const gmm_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<gmm_frame_t*>(encoded);
    unsigned int i;

    frame->timestamp = data->timestampInMsec;
    for (i = 0; i < sizeof(frame->counts); i++) {
        frame->counts[i] = data->hitCounts.byIndex[i];
    }

    return sizeof(gmm_frame_t);
}

void decodeGMMFrame(const uint8_t* encoded, gmm_data_t* decoded)
{
    auto frame = reinterpret_cast<const gmm_frame_t*>(encoded);
    unsigned int i;

    decoded->timestampInMsec = frame->timestamp;
    for (i = 0; i < sizeof(frame->counts); i++) {
        decoded->hitCounts.byIndex[i] = frame->counts[i];
    }
}

}
