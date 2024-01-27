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

/**
 * @brief Envelope of a telemetry frame.
 *
 * The envelope contains a sequence number, the type of the telemetry frame,
 * the source component that sends the frame and the target component the
 * frame is addressed to.
 */
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

/**
 * Returns the minimum size of the buffer that is needed to hold a message with
 * the given payload length.
 *
 * @param payload_length  the payload length of the message
 * @return the minimum size of the buffer; basically the payload length plus the
 *   message framing overhead. Returns zero if the payload is too large.
 */
uint8_t getMessageSizeForPayloadLength(std::uint8_t payload_length);

/**
 * @brief Serializes a telemetry message into a pre-allocated buffer.
 *
 * @param buffer  the buffer to write the serialized message into
 * @param buffer_length  the allocated size of the buffer
 * @param envelope  the envelope of the telemetry message
 * @param payload  the payload of the telemetry message
 * @param payload_length  the length of the payload of the telemetry message
 * @return the number of bytes actually used from the buffer; zero if there
 *   was an error while serializing the message.
 */
[[nodiscard]] std::uint8_t serialize(
    uint8_t* buffer, std::uint8_t buffer_length, envelope_t envelope,
    const std::uint8_t* payload, std::uint8_t payload_length);

}
