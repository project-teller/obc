#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

#include "core/telem.h"
#include "core/utils/crc.h"

using namespace std;
using namespace teller::telem;

#define RXSM_SODS_BIT_INDEX 0
#define RXSM_SOE_BIT_INDEX 1
#define RXSM_LO_BIT_INDEX 2

#define SUBSYSTEM_GMM_BIT_INDEX 0
#define SUBSYSTEM_SCM_BIT_INDEX 2
#define SUBSYSTEM_ADS_BIT_INDEX 4
#define SUBSYSTEM_IMU_BIT_INDEX 6
#define SUBSYSTEM_MAG_BIT_INDEX 8

namespace teller::telem::frames {

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

}

uint8_t teller::telem::getMessageSizeForPayloadLength(uint8_t payload_length)
{
    return payload_length <= MAX_PAYLOAD_LENGTH ? payload_length + 8 : 0;
}

uint8_t teller::telem::serialize(
    uint8_t* buffer, uint8_t buffer_length,
    envelope_t envelope, const uint8_t* payload, uint8_t payload_length)
{
    uint8_t space_needed = getMessageSizeForPayloadLength(payload_length);
    uint16_t crc;

    if (space_needed == 0 || space_needed > buffer_length) {
        return 0;
    }

    if (envelope.source == UNKNOWN_COMPONENT) {
        envelope.source = ONBOARD_COMPUTER;
    }

    if (envelope.target == UNKNOWN_COMPONENT) {
        envelope.target = GROUND_STATION;
    }

    buffer[0] = 0xCA;
    buffer[1] = 0xFE;
    buffer[2] = envelope.seq_no;
    buffer[3] = envelope.frame_type;
    buffer[4] = (((static_cast<int>(envelope.source) & 0x03) << 4) | (static_cast<int>(envelope.target) & 0x03));
    buffer[5] = payload_length;
    memcpy(buffer + 6, payload, payload_length);

    crc = crc_ccitt(0, buffer, payload_length + 6);
    buffer[payload_length + 6] = crc & 0xff;
    buffer[payload_length + 7] = crc >> 8;

    return space_needed;
}

uint8_t teller::telem::frames::encodeHeartbeatFrame(const heartbeat_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<heartbeat_frame_t*>(encoded);

    frame->timestamp = data->timestampInMsec;
    frame->error = data->error;

    /* Board voltage is somewhere between 0 and 5V so we can use a resolution of 0.1V */
    frame->voltage = clamp(
        static_cast<int>(roundf(clamp(data->voltageInVolts, 0.0f, 25.5f) * 10.0f)),
        0, 255);

    /* Temperature measurements shall be made between -30 and 85 degrees
     * according to the requirements, with a resolution of 1 degree, so we are
     * fine with encoding them as signed integers between -128 and 127 degrees Celsius */
    frame->temperature = clamp(
        static_cast<int>(roundf(clamp(data->temperateInCelsius, -128.0f, 127.0f))),
        -128, 127);

    /* clang-format off */
    frame->rxsmStatusBits = (
        (data->rxsmStatusBits.lo ? (1 << RXSM_LO_BIT_INDEX) : 0) |
        (data->rxsmStatusBits.sods ? (1 << RXSM_SODS_BIT_INDEX) : 0) |
        (data->rxsmStatusBits.soe ? (1 << RXSM_SOE_BIT_INDEX) : 0) |
        0
    );
    /* clang-format on */

    /* clang-format off */
    frame->subsystemStatus = (
        (data->subsystemStatus.gmm << SUBSYSTEM_GMM_BIT_INDEX) |
        (data->subsystemStatus.scm << SUBSYSTEM_SCM_BIT_INDEX) |
        (data->subsystemStatus.ads << SUBSYSTEM_ADS_BIT_INDEX) |
        (data->subsystemStatus.imu << SUBSYSTEM_IMU_BIT_INDEX) |
        (data->subsystemStatus.mag << SUBSYSTEM_MAG_BIT_INDEX) |
        0
    );
    /* clang-format on */

    return sizeof(heartbeat_frame_t);
}

void teller::telem::frames::decodeHeartbeatFrame(const uint8_t* encoded, heartbeat_data_t* decoded)
{
    auto frame = reinterpret_cast<const heartbeat_frame_t*>(encoded);
    uint16_t status;

    decoded->timestampInMsec = frame->timestamp;
    decoded->error = frame->error;
    decoded->voltageInVolts = frame->voltage / 10.0f;
    decoded->temperateInCelsius = frame->temperature;

    decoded->rxsmStatusBits.lo = frame->rxsmStatusBits & (1 << RXSM_LO_BIT_INDEX);
    decoded->rxsmStatusBits.sods = frame->rxsmStatusBits & (1 << RXSM_SODS_BIT_INDEX);
    decoded->rxsmStatusBits.soe = frame->rxsmStatusBits & (1 << RXSM_SOE_BIT_INDEX);

    status = frame->subsystemStatus;
    decoded->subsystemStatus.gmm = static_cast<subsystem_status_t>(
        (status >> SUBSYSTEM_GMM_BIT_INDEX) & 0x03);
    decoded->subsystemStatus.scm = static_cast<subsystem_status_t>(
        (status >> SUBSYSTEM_SCM_BIT_INDEX) & 0x03);
    decoded->subsystemStatus.ads = static_cast<subsystem_status_t>(
        (status >> SUBSYSTEM_ADS_BIT_INDEX) & 0x03);
    decoded->subsystemStatus.imu = static_cast<subsystem_status_t>(
        (status >> SUBSYSTEM_IMU_BIT_INDEX) & 0x03);
    decoded->subsystemStatus.mag = static_cast<subsystem_status_t>(
        (status >> SUBSYSTEM_MAG_BIT_INDEX) & 0x03);
}

uint8_t teller::telem::frames::encodeTextMessageFrame(
    const text_message_data_t* data, uint8_t* encoded)
{
    size_t length = strlen(data->message);

    assert(length <= MAX_PAYLOAD_LENGTH - 1);

    encoded[0] = (data->level & 0x07) | (data->module << 3);
    memcpy(encoded + 1, data->message, length);

    return length + 1;
}

void teller::telem::frames::decodeTextMessageFrame(
    const uint8_t* encoded, size_t length, text_message_data_t* decoded)
{
    assert(length <= MAX_PAYLOAD_LENGTH);

    decoded->level = static_cast<log_level_t>(encoded[0] & 0x07);
    decoded->module = static_cast<module_id_t>(encoded[0] >> 3);

    memcpy(decoded->message, encoded + 1, length - 1);
    decoded->message[length - 1] = 0;
}
