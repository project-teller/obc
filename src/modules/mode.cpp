#include "modules/mode.h"

using namespace teller::telem;

/** Current mode of the experiment */
static obc_mode_t currentMode;

namespace teller::mode {

bool init()
{
    currentMode = OBC_MODE_MISSION;
    return true;
}

void destroy()
{
}

obc_mode_t getMode()
{
    return currentMode;
}

}
