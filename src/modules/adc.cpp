#include "modules/adc.h"
#include "modules/log.h"

#include "core/telem/generic.h"
#include "drivers/adc.h"
#include "hal/system.h"

using namespace teller::adc;
using namespace teller::log;
using namespace teller::telem;
using teller::telem::storage_area_t;

/** The number of ADC channels to use */
static const uint8_t NUM_CHANNELS = 13;

/** The raw measurements from the ADC */
static uint16_t rawValues[NUM_CHANNELS];

/** The scaled measurements from the ADC */
static float values[NUM_CHANNELS];

/** Reference level of the ADC that corresponds to the maximum value */
static const float reference = 2.5f;

/** Increment corresponding to the LSB of the ADC */
static const float lsbIncrement = reference / 255;

/** Scaling factors for the channels */
static const float scaling[NUM_CHANNELS] = {
    1.0f, /* CUR_SUC_LCL1 */
    1.0f, /* CUR_SUC_LCL2 */
    1.0f, /* CUR_SUC_LCL3 */
    1.0f, /* CUR_CAM */
    1.0f, /* CUR_SCM */
    1.0f, /* CUR_GMM */
    31.0f, /* 60V_MEAS1 */
    31.0f, /* 60V_MEAS2 */
    31.0f, /* 60V_MEAS3 */
    43.0f / 3.0f, /* 28V_MEAS */
    8.5f, /* 12V_MEAS */
    2.5f, /* 5V_MEAS */
    2.0f, /* 3.3V_MEAS */
};

static Logger* logger;

namespace teller::adc {

bool init()
{
    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;
}

void destroy()
{
}

bool setup(void)
{
    return teller::drivers::adc::setup();
}

bool update()
{
    static int counter = 0;
    uint8_t i;

    if (!teller::drivers::adc::update(NUM_CHANNELS, rawValues)) {
        return false;
    }

    for (i = 0; i < NUM_CHANNELS; i++) {
        values[i] = rawValues[i] * lsbIncrement * scaling[i];
    }

    counter++;
    if (counter >= 10) {
        counter = 0;

        logger->info(
            "ADC: 5V = %f, 3.3V = %f", values[11], values[12]);
    }

    return true;
}

}
