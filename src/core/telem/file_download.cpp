#include <cassert>
#include <cstring>

#include "core/telem/file_download.h"

using namespace std;
using namespace teller::telem;

#define MAX_READ_LENGTH 8192
#define LENGTH_MASK (MAX_READ_LENGTH - 1)

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a file download request in the telemetry protocol.
 *
 * This structure represents the wire encoding of the directory listing request
 * frame. There is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t area;
    uint32_t start;
    uint16_t length;
} file_download_request_frame_t;

#define PREAMBLE_LENGTH sizeof(file_download_request_frame_t)

uint8_t encodeFileDownloadRequestFrame(
    const file_download_request_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<file_download_request_frame_t*>(encoded);
    size_t length = strlen(data->path);

    assert(length <= MAX_PAYLOAD_LENGTH - PREAMBLE_LENGTH);

    frame->area = (data->area & 0x0F);
    frame->start = data->start;
    frame->length = (data->length == MAX_READ_LENGTH ? 0 : data->length) & LENGTH_MASK;

    memcpy(encoded + PREAMBLE_LENGTH, data->path, length);

    return PREAMBLE_LENGTH + length;
}

void decodeFileDownloadRequestFrame(
    const uint8_t* encoded, size_t length, file_download_request_data_t* decoded)
{
    auto frame = reinterpret_cast<const file_download_request_frame_t*>(encoded);
    uint8_t area = frame->area & 0x0F;

    decoded->area = area < NUM_STORAGE_AREAS
        ? static_cast<storage_area_t>(area)
        : STORAGE_AREA_UNKNOWN;
    decoded->start = frame->start;
    decoded->length = (frame->length & LENGTH_MASK);
    if (decoded->length == 0) {
        decoded->length = MAX_READ_LENGTH;
    }

    memcpy(decoded->path, encoded + PREAMBLE_LENGTH, length - PREAMBLE_LENGTH);
    decoded->path[length - PREAMBLE_LENGTH] = 0;
}

bool validateEncodedFileDownloadRequestFrame(const uint8_t* encoded, size_t length)
{
    if (length < sizeof(file_download_request_frame_t)) {
        return false;
    }

    auto frame = reinterpret_cast<const file_download_request_frame_t*>(encoded);
    return frame->length < MAX_READ_LENGTH;
}

}
