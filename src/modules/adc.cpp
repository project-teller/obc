#include <cstring>

#include "core/telem/generic.h"

#include "drivers/adc.h"

#include "hal/board.h"
#include "hal/system.h"

#include "modules/adc.h"
#include "modules/log.h"

using namespace teller::adc;
using namespace teller::log;
using namespace teller::telem;
using teller::telem::storage_area_t;

/** The number of ADC channels to use */
static const uint8_t NUM_CHANNELS = 13;

/** The raw measurements from the ADC */
static uint16_t rawValues[NUM_CHANNELS];

/** The timestamp of the last raw measurements */
static uint32_t lastMeasurementTakenAt = 0;

/** The scaled measurements from the ADC */
static float scaledValues[NUM_CHANNELS];

/** Reference level of the ADC that corresponds to the maximum value */
static const float reference = 2.5f;

/** Increment corresponding to the LSB of the ADC */
static const float lsbIncrement = reference / 256;

/** Scaling factors for the channels */
static const float scaling[NUM_CHANNELS] = {
    1.f / 20.f, /* CUR_SUC_LCL1 */
    1.f / 20.f, /* CUR_SUC_LCL2 */
    1.f / 20.f, /* CUR_SUC_LCL3 */
    1.f, /* CUR_CAM */
    0.5f, /* CUR_SCM */
    0.1f, /* CUR_GMM */
    31.0f, /* 60V_MEAS1 */
    31.0f, /* 60V_MEAS2 */
    31.0f, /* 60V_MEAS3 */
    43.0f / 3.0f, /* 28V_MEAS */
    9.07f, /* 12V_MEAS */
    2.5f, /* 5V_MEAS */
    2.f, /* 3V3_MEAS */
};

static Logger* logger;

namespace teller::adc {

bool init()
{
    lastMeasurementTakenAt = 0;

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        rawValues[i] = 0;
        scaledValues[i] = 0.0f;
    }

    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;
}

void destroy()
{
}

bool setup(void)
{
    lastMeasurementTakenAt = 0;
    return teller::drivers::adc::setup();
}

bool update()
{
    if (!teller::drivers::adc::update(NUM_CHANNELS, rawValues)) {
        return false;
    }

    lastMeasurementTakenAt = teller::hal::system::getTimeSinceBootMsec();
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        scaledValues[i] = rawValues[i] * lsbIncrement * scaling[i];
    }

    // Forward the value corresponding to the 3.3V rail to the board module
    teller::hal::board::updateBoardVoltage(scaledValues[12]);

    return true;
}

void getMeasurements(uint32_t& timestampMsec, float* values)
{
    timestampMsec = lastMeasurementTakenAt;
    memcpy(values, scaledValues, NUM_CHANNELS * sizeof(float));
}

}
