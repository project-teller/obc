#include "modules/mode.h"
#include "hal/event_flags.hpp"
#include "hal/uart.h"

using teller::hal::EventFlags;
using namespace teller::telem;

/** Current mode of the experiment */
static obc_mode_t currentMode;

/** Stores whether the ground station requested testing mode */
static bool testRequestedFromGND;

/** Event flags that are triggered when a mode change is requested */
static EventFlags modeChangeReasons;

/** Updates the current mode of the experiment */
static void setMode(obc_mode_t mode);

namespace teller::mode {

bool init()
{
    testRequestedFromGND = false;
    setMode(OBC_MODE_MISSION);
    notifyPossibleModeChange(MODE_CHANGE_REASON_OTHER);

    return true;
}

void destroy()
{
}

obc_mode_t getMode()
{
    return currentMode;
}

void notifyPossibleModeChange(ModeChangeReasons reason)
{
    modeChangeReasons.set(reason);
}

void updateMode()
{
    uint32_t reasons = modeChangeReasons.waitAny(MODE_CHANGE_REASON_ANY);
    obc_mode_t newMode = currentMode;

    if (reasons & MODE_CHANGE_REASON_GND_TEST_START) {
        testRequestedFromGND = true;
    } else if (reasons & MODE_CHANGE_REASON_GND_TEST_STOP) {
        testRequestedFromGND = false;
    }

    newMode = testRequestedFromGND || teller::hal::uart::isConnected(teller::hal::uart::DEBUG)
        ? OBC_MODE_TESTING
        : OBC_MODE_MISSION;

    setMode(newMode);
}

}

static void setMode(obc_mode_t mode)
{
    currentMode = mode;
}
