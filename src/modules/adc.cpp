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
static float measurements[NUM_CHANNELS];

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

    if (!teller::drivers::adc::update(NUM_CHANNELS, measurements)) {
        return false;
    }

    counter++;
    if (counter >= 10) {
        counter = 0;

        logger->info("ADC: %f %f", measurements[11], measurements[12]);
    }

    return true;
}

}
