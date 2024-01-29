#pragma once

#include <cstdint>

namespace teller::telem {

/** Maximum length of payload allowed in a telemetry message, inclusive */
const int MAX_PAYLOAD_LENGTH = 63;

/** Maximum size of a single telemetry message, inclusive */
const int MAX_MESSAGE_LENGTH = 71;

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

namespace frames {

    /**
     * @brief Enum representing the frame types in the telemetry protocol.
     */
    typedef enum {
        UNKNOWN = 0,
        HEARTBEAT = 1,
        TEXT_MESSAGE = 2,
    } frame_type_t;

    /**
     * @brief Structure containing all the data required to construct a heartbeat frame.
     *
     * This structure is not the same as the wire encoding of the heartbeat
     * frame. It contains the raw values of the fields, in SI units where
     * applicable.
     *
     * @see heartbeat_frame_t
     */
    typedef struct {
        uint32_t timestampInMsec;
        uint8_t error;
        float voltageInVolts;
        float temperateInCelsius;
        struct {
            bool sods;
            bool soe;
            bool lo;
        } rxsmStatusBits;
        struct {
            subsystem_status_t gmm;
            subsystem_status_t scm;
            subsystem_status_t ads;
            subsystem_status_t imu;
            subsystem_status_t mag;
        } subsystemStatus;
    } heartbeat_data_t;

    /**
     * @brief Structure of the payload of a heartbeat frame in the telemetry protocol.
     *
     * This structure represents the wire encoding of the heartbeat frame. There
     * is another structure for the raw values.
     */
    typedef struct __attribute__((packed)) {
        uint32_t timestamp;
        uint8_t error;
        uint8_t voltage;
        int8_t temperature;
        uint8_t rxsmStatusBits;
        uint16_t subsystemStatus;
    } heartbeat_frame_t;

    static_assert(sizeof(heartbeat_frame_t) == 10, "Heartbeat frame size invalid");

    /**
     * @brief Encodes a heartbeat frame into its wire representation.
     *
     * @param data  the data to encode in the telemetry frame
     * @param encoded  the encoded representation of the telemetry frame
     */
    void encodeHeartbeatFrame(const heartbeat_data_t* data, heartbeat_frame_t* encoded);

    /**
     * @brief Decodes a heartbeat frame from its wire representation.
     *
     * @param encoded  the encoded representation of the telemetry frame
     * @param decoded  the decoded representation of the telemetry frame
     */
    void decodeHeartbeatFrame(const heartbeat_frame_t* encoded, heartbeat_data_t* decoded);
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
