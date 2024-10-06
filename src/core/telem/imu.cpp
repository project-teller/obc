#include "core/telem/imu.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of an IMU frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the IMU frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    float accX;
    float accY;
    float accZ;
    float gyrX;
    float gyrY;
    float gyrZ;
} imu_frame_t;

static_assert(sizeof(imu_frame_t) == 28, "IMU frame size invalid");

uint8_t encodeIMUFrame(const imu_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<imu_frame_t*>(encoded);

    frame->timestamp = data->timestampInMsec;
    frame->accX = data->acceleration.x;
    frame->accY = data->acceleration.y;
    frame->accZ = data->acceleration.z;
    frame->gyrX = data->angularVelocity.x;
    frame->gyrY = data->angularVelocity.y;
    frame->gyrZ = data->angularVelocity.z;

    return sizeof(imu_frame_t);
}

void decodeIMUFrame(const uint8_t* encoded, imu_data_t* decoded)
{
    auto frame = reinterpret_cast<const imu_frame_t*>(encoded);

    decoded->timestampInMsec = frame->timestamp;
    decoded->acceleration.x = frame->accX;
    decoded->acceleration.y = frame->accY;
    decoded->acceleration.z = frame->accZ;
    decoded->angularVelocity.x = frame->gyrX;
    decoded->angularVelocity.y = frame->gyrY;
    decoded->angularVelocity.z = frame->gyrZ;
}

bool validateEncodedIMUFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(imu_frame_t);
}

}
