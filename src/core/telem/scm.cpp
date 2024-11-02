#include <cstring>

#include "core/telem/scm.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of an SCM frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the SCM frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint8_t maxFragmentIndex;
    uint8_t fragmentIndex;
    uint8_t scintillatorIndex;
    uint8_t data[MAX_SCM_FRAME_FRAGMENT_LENGTH];
} scm_frame_t;

#define HEADER_LENGTH (offsetof(scm_frame_t, data))

uint8_t encodeSCMFrame(const scm_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<scm_frame_t*>(encoded);
    unsigned int i;

    frame->timestamp = data->timestampInMsec;
    frame->maxFragmentIndex = data->maxFragmentIndex;
    frame->fragmentIndex = data->fragmentIndex;
    frame->scintillatorIndex = data->scintillatorIndex;
    memcpy(frame->data, data->data, data->length);

    return HEADER_LENGTH + data->length;
}

void decodeSCMFrame(const uint8_t* encoded, size_t length, scm_data_t* decoded)
{
    auto frame = reinterpret_cast<const scm_frame_t*>(encoded);
    unsigned int i;

    decoded->timestampInMsec = frame->timestamp;
    decoded->maxFragmentIndex = frame->maxFragmentIndex;
    decoded->fragmentIndex = frame->fragmentIndex;
    decoded->scintillatorIndex = frame->scintillatorIndex;
    decoded->length = length >= HEADER_LENGTH ? length - HEADER_LENGTH : 0;
    memcpy(decoded->data, frame->data, decoded->length);
}

bool validateEncodedSCMFrame(const uint8_t* encoded, size_t length)
{
    return length >= HEADER_LENGTH;
}

}
