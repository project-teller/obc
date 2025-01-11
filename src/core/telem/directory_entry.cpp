#include "core/telem/directory_entry.h"

#include <cstring>

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a binary data frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the binary data frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t frame_type;
    uint8_t seq_no;
    uint8_t max_entry_index;
    uint8_t entry_index;
} directory_entry_frame_t;

#define PREAMBLE_LENGTH sizeof(directory_entry_frame_t)

uint8_t encodeDirectoryEntryFrame(const directory_entry_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<directory_entry_frame_t*>(encoded);
    size_t length = strlen(data->name);

    frame->frame_type = data->frame_type;
    frame->seq_no = data->seq_no;
    frame->max_entry_index = data->max_entry_index;
    frame->entry_index = data->entry_index;

    memcpy(encoded + PREAMBLE_LENGTH, data->name, length);

    return PREAMBLE_LENGTH + length;
}

void decodeDirectoryEntryFrame(const uint8_t* encoded, size_t length, directory_entry_data_t* decoded)
{
    auto frame = reinterpret_cast<const directory_entry_frame_t*>(encoded);

    decoded->frame_type = static_cast<frame_type_t>(frame->frame_type);
    decoded->seq_no = frame->seq_no;
    decoded->max_entry_index = frame->max_entry_index;
    decoded->entry_index = frame->entry_index;

    memcpy(decoded->name, encoded + PREAMBLE_LENGTH, length - PREAMBLE_LENGTH);
    decoded->name[length - PREAMBLE_LENGTH] = 0;
}

bool validateEncodedDirectoryEntryFrame(const uint8_t* encoded, size_t length)
{
    if (length < sizeof(directory_entry_frame_t)) {
        return false;
    }

    auto frame = reinterpret_cast<const directory_entry_frame_t*>(encoded);
    return frame->max_entry_index >= frame->entry_index;
}

}
