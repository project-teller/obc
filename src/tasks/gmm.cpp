#include "tasks/gmm.h"

#include "hal/system.h"
#include "modules/gmm.h"
#include "modules/supervisor.h"
#include "modules/telem.h"

using namespace teller;
using namespace teller::supervisor;
using namespace teller::telem;

/**
 * @def BASE_GMM_UPDATE_FREQ_HZ
 * @brief Specifies the base update frequency that we expect from the GMM.
 */
#define BASE_GMM_UPDATE_FREQ_HZ 50

[[noreturn]] void teller::tasks::gmmTask(void* arg)
{
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    bool healthy, updated;

    while (true) {
        gmm::setup();

        healthy = true;
        while (healthy) {
            healthy = gmm::update(payload, updated);
        }

        /* No GMM measurements received for a while, wait a bit and then
         * retry */
        hal::system::delayMsec(1000);
    }
}
