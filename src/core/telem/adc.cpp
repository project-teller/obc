#include <cstring>

#include "core/telem/adc.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of an ADC frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the ADC frame. There
 * is another structure for the raw values.
 *
 * Currents are encoded in units of 1/10 mA. Voltages are encoded in units of
 * 10 mV.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    uint16_t measurements[13];
} adc_frame_t;

uint8_t encodeADCFrame(const adc_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<adc_frame_t*>(encoded);
    unsigned int i;

    frame->timestamp = data->timestampInMsec;

    /* Encoding currents */
    for (i = 0; i < 6; i++) {
        frame->measurements[i] = data->measurements.byIndex[i] * 10000.0f;
    }

    /* Encoding voltages */
    for (i = 7; i < 13; i++) {
        frame->measurements[i] = data->measurements.byIndex[i] * 100.0f;
    }

    return sizeof(adc_frame_t);
}

void decodeADCFrame(const uint8_t* encoded, adc_data_t* decoded)
{
    auto frame = reinterpret_cast<const adc_frame_t*>(encoded);
    unsigned int i;

    decoded->timestampInMsec = frame->timestamp;

    /* Decoding currents */
    for (i = 0; i < 6; i++) {
        decoded->measurements.byIndex[i] = frame->measurements[i] / 10000.0f;
    }

    /* Decoding voltages */
    for (i = 0; i < 6; i++) {
        decoded->measurements.byIndex[i] = frame->measurements[i] / 100.0f;
    }
}

bool validateEncodedADCFrame(const uint8_t* encoded, size_t length)
{
    return length >= sizeof(adc_frame_t);
}

}
