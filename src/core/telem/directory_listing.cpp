#include <cassert>
#include <cstring>

#include "core/telem/directory_listing.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a directory listing request in the telemetry protocol.
 *
 * This structure represents the wire encoding of the directory listing request
 * frame. There is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t area;
    uint16_t start;
    uint16_t count;
} directory_listing_request_frame_t;

#define PREAMBLE_LENGTH sizeof(directory_listing_request_frame_t)

uint8_t encodeDirectoryListingRequestFrame(
    const directory_listing_request_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<directory_listing_request_frame_t*>(encoded);
    size_t length = strlen(data->name);

    assert(length <= MAX_PAYLOAD_LENGTH - PREAMBLE_LENGTH);

    frame->area = (data->area & 0x0F);
    frame->start = data->start;
    frame->count = data->count;
    memcpy(encoded + PREAMBLE_LENGTH, data->name, length);

    return PREAMBLE_LENGTH + length;
}

void decodeDirectoryListingRequestFrame(
    const uint8_t* encoded, size_t length, directory_listing_request_data_t* decoded)
{
    auto frame = reinterpret_cast<const directory_listing_request_frame_t*>(encoded);
    uint8_t area = frame->area & 0x0F;

    decoded->area = area < NUM_STORAGE_AREAS
        ? static_cast<storage_area_t>(area)
        : STORAGE_AREA_UNKNOWN;
    decoded->start = frame->start;
    decoded->count = frame->count;

    memcpy(decoded->name, encoded + PREAMBLE_LENGTH, length - PREAMBLE_LENGTH);
    decoded->name[length - PREAMBLE_LENGTH] = 0;
}

bool validateEncodedDirectoryListingRequestFrame(const uint8_t* encoded, size_t length)
{
    if (length < sizeof(directory_listing_request_frame_t)) {
        return false;
    }

    auto frame = reinterpret_cast<const directory_listing_request_frame_t*>(encoded);
    return frame->count <= 256; /* because the response has a one-byte entry index */
}

}
