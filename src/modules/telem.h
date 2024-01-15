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

/**
 * Initializes the data structures required by the telemetry module.
 *
 * @return whether the initialization was successful
 */
bool init(void);

/**
 * @brief Flushes the next message waiting in the telemetry subsystem to the
 * appropriate UART.
 *
 * Blocks indefinitely if there are no messages to send.
 */
void flushNext(void);

/**
 * @brief Sends a raw byte sequence to the telemetry module.
 *
 * The data being sent here is copied before it is enqueued for the telemetry
 * module. The ownership of the original buffer remains at the caller. The
 * copy will be freed by the telemetry module.
 *
 * @param data    the raw data to send
 * @param length  lengrh of the data to send
 * @return whether the data was sent successfully to the telemetry module
 */
bool send(const std::uint8_t* data, std::uint16_t length);

/**
 * @brief Sends a string to the telemetry module.
 *
 * The data being sent here is copied before it is enqueued for the telemetry
 * module. The ownership of the original buffer remains at the caller. The
 * copy will be freed by the telemetry module.
 *
 * @param data    the string to send
 * @return whether the data was sent successfully to the telemetry module
 */
bool send(const char* data);

/**
 * @brief Sends a telemetry message to the telemetry module, using the given envelope.
 *
 * The message is defined by its envelope (containing a sequence number, a
 * message type, a source component ID and a target component ID) and the
 * payload. In the envelope, the caller needs to provide the message type and
 * \em may provide the target component ID. When the target component ID is
 * unknown (unspecified), it is assumed to be the ground station. When the
 * source component ID is unknown (unspecified), it is assumed to be the
 * onboard computer.
 *
 * Sequence numbers will be filled automatically by the telemetry module.
 *
 * @param envelope  the envelope of the telemetry message
 * @param payload   the payload of the message
 * @param length    lengrh of the payload to send
 * @return whether the data was sent successfully to the telemetry module
 */
bool send(envelope_t envelope, const std::uint8_t* payload, std::uint16_t length);

/**
 * @brief Sends a telemetry message of a given type to the telemetry module.
 *
 * This is a convenience function that fills the envelope with sensible defaults.
 * Only the message type needs to be provided explicitly.
 *
 * @param type      the type of the message
 * @param payload   the payload of the message
 * @param length    lengrh of the payload to send
 * @return whether the data was sent successfully to the telemetry module
 */
bool send(frames::frame_type_t type, const std::uint8_t* payload, std::uint16_t length);

}
