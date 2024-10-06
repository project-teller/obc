#include <algorithm>
#include <cmath>

#include "core/telem/heartbeat.h"

using namespace std;
using namespace teller::telem;

#define RXSM_SODS_BIT_INDEX 0
#define RXSM_SOE_BIT_INDEX 1
#define RXSM_LO_BIT_INDEX 2
#define MODE_BIT_INDEX 7

#define SUBSYSTEM_GMM_BIT_INDEX 0
#define SUBSYSTEM_SCM_BIT_INDEX 2
#define SUBSYSTEM_ADS_BIT_INDEX 4
#define SUBSYSTEM_IMU_BIT_INDEX 6
#define SUBSYSTEM_MAG_BIT_INDEX 8
#define SUBSYSTEM_STO_BIT_INDEX 10

#define LCL_STATUS_GMM_BIT_INDEX 0
#define LCL_STATUS_SCM_BIT_INDEX 1
#define LCL_STATUS_SUC1_BIT_INDEX 2
#define LCL_STATUS_SUC2_BIT_INDEX 3
#define LCL_STATUS_SUC3_BIT_INDEX 4
#define LCL_STATUS_CAM_BIT_INDEX 5

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
    uint8_t lclStatusBits;
} heartbeat_frame_t;

static_assert(sizeof(heartbeat_frame_t) == 11, "Heartbeat frame size invalid");

uint8_t encodeHeartbeatFrame(const heartbeat_data_t* data, uint8_t* encoded)
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
        static_cast<int>(roundf(clamp(data->temperatureInCelsius, -128.0f, 127.0f))),
        -128, 127);

    /* clang-format off */
    frame->rxsmStatusBits = (
        (data->rxsmStatusBits.lo ? (1 << RXSM_LO_BIT_INDEX) : 0) |
        (data->rxsmStatusBits.sods ? (1 << RXSM_SODS_BIT_INDEX) : 0) |
        (data->rxsmStatusBits.soe ? (1 << RXSM_SOE_BIT_INDEX) : 0) |
        (data->mode == OBC_MODE_MISSION ? 0 : (1 << MODE_BIT_INDEX)) |
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
        (data->subsystemStatus.sto << SUBSYSTEM_STO_BIT_INDEX) |
        0
    );
    /* clang-format on */

    /* clang-format off */
    frame->lclStatusBits = (
        (data->lclStatusBits.gmm << LCL_STATUS_GMM_BIT_INDEX) |
        (data->lclStatusBits.scm << LCL_STATUS_SCM_BIT_INDEX) |
        (data->lclStatusBits.suc1 << LCL_STATUS_SUC1_BIT_INDEX) |
        (data->lclStatusBits.suc2 << LCL_STATUS_SUC2_BIT_INDEX) |
        (data->lclStatusBits.suc3 << LCL_STATUS_SUC3_BIT_INDEX) |
        (data->lclStatusBits.cam << LCL_STATUS_CAM_BIT_INDEX) |
        0
    );
    /* clang-format on */

    return sizeof(heartbeat_frame_t);
}

void decodeHeartbeatFrame(const uint8_t* encoded, heartbeat_data_t* decoded)
{
    auto frame = reinterpret_cast<const heartbeat_frame_t*>(encoded);
    uint16_t status;

    decoded->timestampInMsec = frame->timestamp;
    decoded->error = frame->error;
    decoded->voltageInVolts = frame->voltage / 10.0f;
    decoded->temperatureInCelsius = frame->temperature;

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
    decoded->subsystemStatus.sto = static_cast<subsystem_status_t>(
        (status >> SUBSYSTEM_STO_BIT_INDEX) & 0x03);

    decoded->lclStatusBits.gmm = frame->lclStatusBits & (1 << LCL_STATUS_GMM_BIT_INDEX);
    decoded->lclStatusBits.scm = frame->lclStatusBits & (1 << LCL_STATUS_SCM_BIT_INDEX);
    decoded->lclStatusBits.suc1 = frame->lclStatusBits & (1 << LCL_STATUS_SUC1_BIT_INDEX);
    decoded->lclStatusBits.suc2 = frame->lclStatusBits & (1 << LCL_STATUS_SUC2_BIT_INDEX);
    decoded->lclStatusBits.suc3 = frame->lclStatusBits & (1 << LCL_STATUS_SUC3_BIT_INDEX);
    decoded->lclStatusBits.cam = frame->lclStatusBits & (1 << LCL_STATUS_CAM_BIT_INDEX);
}

bool validateEncodedHeartbeatFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(heartbeat_frame_t);
}

}
