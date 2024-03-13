#pragma once

#include <cstdint>
#include <cstdlib>

namespace teller::telem {

/** Length of the header of a telemetry message */
const int HEADER_LENGTH = 6;

/** Maximum length of payload allowed in a telemetry message, inclusive */
const int MAX_PAYLOAD_LENGTH = 63;

/** Maximum size of a single telemetry message, inclusive */
const int MAX_MESSAGE_LENGTH = MAX_PAYLOAD_LENGTH + HEADER_LENGTH + 2;

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

/**
 * @brief Enum representing the possible status codes of subsystems.
 */
typedef enum {
    SUBSYSTEM_STATUS_CRITICAL = 0,
    SUBSYSTEM_STATUS_ERROR = 1,
    SUBSYSTEM_STATUS_WARNING = 2,
    SUBSYSTEM_STATUS_OK = 3
} subsystem_status_t;

/**
 * @brief Enum containing the IDs of the modules used in text messages.
 */
typedef enum {
    MODULE_ID_GENERIC = 0,
    MODULE_ID_OBC = 1,
    MODULE_ID_GMM = 2,
    MODULE_ID_SCM = 3,
    MODULE_ID_ADS = 4,
    MODULE_ID_IMU = 5,
    MODULE_ID_MAG = 6,
    MODULE_COUNT,
} module_id_t;

/**
 * @brief Enum containing the log levels used in text messages.
 *
 * The log levels are meant to be compatible with the standard UNIX
 * \c syslog log levels.
 */
typedef enum {
    LOG_LEVEL_EMERGENCY = 0,
    LOG_LEVEL_ALERT = 1,
    LOG_LEVEL_CRITICAL = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_WARNING = 4,
    LOG_LEVEL_NOTICE = 5,
    LOG_LEVEL_INFO = 6,
    LOG_LEVEL_DEBUG = 7,
} log_level_t;

namespace frames {

    /**
     * @brief Enum representing the frame types in the telemetry protocol.
     */
    typedef enum {
        UNKNOWN = 0,
        HEARTBEAT = 1,
        TEXT_MESSAGE = 2,
        CLOCK_STATUS = 3,
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
std::uint8_t getMessageSizeForPayloadLength(std::uint8_t payload_length);

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
    std::uint8_t* buffer, std::uint8_t buffer_length, envelope_t envelope,
    const std::uint8_t* payload, std::uint8_t payload_length);

}
