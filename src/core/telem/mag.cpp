#include "core/telem/mag.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of an MAG frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the MAG frame. There
 * is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    float magX;
    float magY;
    float magZ;
    int16_t temperature;
} mag_frame_t;

static_assert(sizeof(mag_frame_t) == 18, "MAG frame size invalid");

uint8_t encodeMAGFrame(const mag_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<mag_frame_t*>(encoded);
    float normalizedTemperature;

    normalizedTemperature = data->temperature;
    if (normalizedTemperature < -200) {
        normalizedTemperature = -200;
    }
    if (normalizedTemperature > 300) {
        normalizedTemperature = 300;
    }

    frame->timestamp = data->timestampInMsec;
    frame->magX = data->magneticVector.x;
    frame->magY = data->magneticVector.y;
    frame->magZ = data->magneticVector.z;
    frame->temperature = normalizedTemperature * 100.0f;

    return sizeof(mag_frame_t);
}

void decodeMAGFrame(const uint8_t* encoded, mag_data_t* decoded)
{
    auto frame = reinterpret_cast<const mag_frame_t*>(encoded);

    decoded->timestampInMsec = frame->timestamp;
    decoded->magneticVector.x = frame->magX;
    decoded->magneticVector.y = frame->magY;
    decoded->magneticVector.z = frame->magZ;
    decoded->temperature = frame->temperature / 100.0f;
}

bool validateEncodedMAGFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(mag_frame_t);
}

}
