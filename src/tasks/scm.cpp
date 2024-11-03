#include "tasks/scm.h"

#include "hal/system.h"
#include "modules/scm.h"
#include "modules/supervisor.h"
#include "modules/telem.h"

using namespace teller;
using namespace teller::supervisor;
using namespace teller::telem;

/**
 * @def BASE_SCM_UPDATE_FREQ_HZ
 * @brief Specifies the base update frequency that we expect from the GMM.
 */
#define BASE_SCM_UPDATE_FREQ_HZ 10

[[noreturn]] void teller::tasks::scmTask(void* arg)
{
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    bool healthy, updated;
    TaskRegistration task("scm");
    task.expect(BASE_SCM_UPDATE_FREQ_HZ - 2, BASE_SCM_UPDATE_FREQ_HZ + 1);

    healthy = scm::setup();
    if (!healthy) {
        task.disable();
    }

    while (healthy) {
        healthy = scm::update(payload, updated);
        if (updated) {
            task.nudge();
        }
    }

    /* TODO: maybe reset and retry? */
    hal::system::sleepForever();
}
