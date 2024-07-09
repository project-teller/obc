#include "core/telem/calibration.h"

using namespace std;
using namespace teller::telem;

namespace teller::telem::frames {

/**
 * @brief Structure of the payload of a calibration request frame in the telemetry protocol.
 *
 * This structure represents the wire encoding of the calibration request frame.
 * There is another structure for the raw values.
 */
typedef struct __attribute__((packed)) {
    uint8_t procedure;
} calibration_request_frame_t;

uint8_t encodeCalibrationRequestFrame(
    const calibration_request_data_t* data, uint8_t* encoded)
{
    auto frame = reinterpret_cast<calibration_request_frame_t*>(encoded);

    frame->procedure = data->procedure;

    return sizeof(calibration_request_frame_t);
}

void decodeCalibrationRequestFrame(const uint8_t* encoded, calibration_request_data_t* decoded)
{
    auto frame = reinterpret_cast<const calibration_request_frame_t*>(encoded);
    decoded->procedure = frame->procedure < NUM_CALIBRATION_PROCEDURES
        ? static_cast<calibration_procedure_t>(frame->procedure)
        : CALIBRATION_NOP;
}

}
