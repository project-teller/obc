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

    while (true) {
        scm::setup();

        healthy = true;
        while (healthy) {
            healthy = scm::update(payload, updated);
        }

        /* No SCM measurements received for a while, wait a bit and then
         * retry */
        hal::system::delayMsec(1000);
    }
}
