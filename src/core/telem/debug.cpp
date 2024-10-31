#include "core/telem/debug.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a debug command frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the debug command frame.
 * There is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t command;
} debug_command_frame_t;

uint8_t encodeDebugCommandFrame(
    const debug_command_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<debug_command_frame_t*>(encoded);

    frame->command = data->command;

    return sizeof(debug_command_frame_t);
}

void decodeDebugCommandFrame(const uint8_t* encoded, debug_command_data_t* decoded)
{
    auto frame = reinterpret_cast<const debug_command_frame_t*>(encoded);
    decoded->command = frame->command < NUM_DEBUG_COMMANDS
        ? static_cast<debug_command_t>(frame->command)
        : DEBUG_CMD_NOP;
}

bool validateEncodedDebugCommandFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(debug_command_frame_t);
}

}
