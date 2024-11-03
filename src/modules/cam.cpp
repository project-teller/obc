#include "modules/cam.h"
#include "modules/lcl.h"
#include "modules/log.h"

#include "hal/gpio.h"
#include "hal/system.h"

using namespace teller::hal::gpio;
using namespace teller::log;
using namespace teller::telem;

static bool enabled;
static bool inited;
static Logger* logger;

/** Pulse duration to use for toggling the camera status. Used to speed up unit tests. */
static uint16_t pulse_duration_msec;

namespace teller::cam {

bool init()
{
    pulse_duration_msec = 500;

    /* Camera pin starts from off */
    write(START_CAM, 0);
    enabled = false;

    logger = getLogger(MODULE_ID_ADS);
    inited = logger != nullptr;

    return inited;
}

void destroy()
{
    inited = false;
}

subsystem_status_t getSubsystemStatus()
{
    if (!inited) {
        return SUBSYSTEM_STATUS_CRITICAL;
    } else if (!teller::lcl::triggered(teller::lcl::CAM_LCL)) {
        return SUBSYSTEM_STATUS_ERROR;
    } else {
        return enabled ? SUBSYSTEM_STATUS_OK : SUBSYSTEM_STATUS_WARNING;
    }
}

bool isEnabled(void)
{
    /* TODO(ntamas): determine this from the current consumption */
    return enabled;
}

void setEnabled(bool value)
{
    if (isEnabled() != value) {
        sendPulse();
        enabled = value;

        if (value) {
            logger->info("Camera enabled");
        } else {
            logger->info("Camera disabled");
        }
    }
}

void setPulseDurationMsec(uint16_t duration_msec)
{
    pulse_duration_msec = duration_msec;
}

void sendPulse(void)
{
    write(START_CAM, 1);
    teller::hal::system::delayMsec(pulse_duration_msec);
    write(START_CAM, 0);
    teller::hal::system::delayMsec(pulse_duration_msec);
}

}
