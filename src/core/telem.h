#pragma once

#include <cstdint>

namespace teller::telem {

/**
 * Enum representing the possible components that can be addressed by a
 * telemetry message.
 */
typedef enum {
    UNKNOWN_COMPONENT = 0,
    GROUND_STATION = 1,
    ONBOARD_COMPUTER = 2,
    SCINTILLATOR_MODULE = 3,
} component_t;

typedef struct {
    std::uint8_t seq_no;
    std::uint8_t frame_type;
    component_t source;
    component_t target;
} envelope_t;

namespace frames {

    /**
     * Enum representing the frame types in the telemetry protocol.
     */
    typedef enum {
        UNKNOWN = 0,
        HEARTBEAT = 1,
        TEXT_MESSAGE = 2,
    } frame_type_t;

}

uint8_t getMessageSizeForPayloadLength(std::uint8_t payload_length);

[[nodiscard]] std::uint8_t serialize(
    uint8_t* buffer, std::uint8_t buffer_length, envelope_t envelope,
    const std::uint8_t* payload, std::uint8_t payload_length);

}
