#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include "core/telem.h"
#include "core/utils/crc.h"

/** Maximum length of payload allowed in a telemetry message, inclusive */
static const int MAX_PAYLOAD_LENGTH = 63;

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

void teller::telem::frames::encodeHeartbeatFrame(const heartbeat_data_t* data, heartbeat_frame_t* encoded)
{
    encoded->timestamp = data->timestampInMsec;
    encoded->error = data->error;

    /* Board voltage is somewhere between 0 and 5V so we can use a resolution of 0.1V */
    encoded->voltage = clamp(
        static_cast<int>(roundf(clamp(data->voltageInVolts, 0.0f, 25.5f) * 10.0f)),
        0, 255);

    /* Temperature measurements shall be made between -30 and 85 degrees
     * according to the requirements, with a resolution of 1 degree, so we are
     * fine with encoding them as signed integers between -128 and 127 degrees Celsius */
    encoded->temperature = clamp(
        static_cast<int>(roundf(clamp(data->temperateInCelsius, -128.0f, 127.0f))),
        -128, 127);

    /* clang-format off */
    encoded->rxsmStatusBits = (
        (data->rxsmStatusBits.lo ? (1 << RXSM_LO_BIT_INDEX) : 0) |
        (data->rxsmStatusBits.sods ? (1 << RXSM_SODS_BIT_INDEX) : 0) |
        (data->rxsmStatusBits.soe ? (1 << RXSM_SOE_BIT_INDEX) : 0) |
        0
    );
    /* clang-format on */

    /* clang-format off */
    encoded->subsystemStatus = (
        (data->subsystemStatus.gmm << SUBSYSTEM_GMM_BIT_INDEX) |
        (data->subsystemStatus.scm << SUBSYSTEM_SCM_BIT_INDEX) |
        (data->subsystemStatus.ads << SUBSYSTEM_ADS_BIT_INDEX) |
        (data->subsystemStatus.imu << SUBSYSTEM_IMU_BIT_INDEX) |
        (data->subsystemStatus.mag << SUBSYSTEM_MAG_BIT_INDEX) |
        0
    );
    /* clang-format on */
}

void teller::telem::frames::decodeHeartbeatFrame(const heartbeat_frame_t* encoded, heartbeat_data_t* decoded)
{
    uint16_t status;

    decoded->timestampInMsec = encoded->timestamp;
    decoded->error = encoded->error;
    decoded->voltageInVolts = encoded->voltage / 10.0f;
    decoded->temperateInCelsius = encoded->temperature;

    decoded->rxsmStatusBits.lo = encoded->rxsmStatusBits & (1 << RXSM_LO_BIT_INDEX);
    decoded->rxsmStatusBits.sods = encoded->rxsmStatusBits & (1 << RXSM_SODS_BIT_INDEX);
    decoded->rxsmStatusBits.soe = encoded->rxsmStatusBits & (1 << RXSM_SOE_BIT_INDEX);

    status = encoded->subsystemStatus;
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
