#include "core/telem/storage.h"

using namespace std;
using namespace teller::telem;

#define MAX_READ_LENGTH 8192

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a storage area related command in the telemetry protocol.
 *
 * This structure represents the wire encoding of the storage area related command
 * frame. There is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t area_and_command;
    uint32_t offset;
    uint16_t length;
} storage_command_frame_t;

uint8_t encodeStorageCommandFrame(
    const storage_command_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<storage_command_frame_t*>(encoded);

    frame->area_and_command = (data->area << 4) | (data->command & 0x0F);
    frame->offset = data->offset;
    frame->length = data->length == MAX_READ_LENGTH ? 0 : data->length;

    return sizeof(storage_command_frame_t);
}

void decodeStorageCommandFrame(const uint8_t* encoded, storage_command_data_t* decoded)
{
    auto frame = reinterpret_cast<const storage_command_frame_t*>(encoded);
    uint8_t area = frame->area_and_command >> 4;
    uint8_t command = frame->area_and_command & 0x0F;

    decoded->area = area < NUM_STORAGE_AREAS
        ? static_cast<storage_area_t>(area)
        : STORAGE_AREA_UNKNOWN;
    decoded->command = command < NUM_STORAGE_COMMANDS
        ? static_cast<storage_command_t>(command)
        : STORAGE_COMMAND_NOP;
    decoded->offset = frame->offset;
    decoded->length = frame->length ? frame->length : MAX_READ_LENGTH;
}

}
