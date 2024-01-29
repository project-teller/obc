#include <cstring>

#include "hal/system.h"

#include "modules/errors.h"
#include "modules/telem.h"
#include "tasks/serial.h"
#include "tasks/telem.h"

using namespace teller::errors;
using namespace teller::hal::system;
using namespace teller::telem;

const osThreadAttr_t teller::tasks::telemetryTaskAttr = {
    .name = "telem",
    .stack_size = 1024,
    .priority = osPriorityNormal,
};

__NO_RETURN void teller::tasks::telemetryTask(void* arg)
{
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    frames::heartbeat_data_t heartbeat;

    memset(&heartbeat, 0, sizeof(heartbeat));

    for (;;) {
        heartbeat.timestampInMsec = getTimeSinceBootMsec();

        /* TODO: get board voltage and temperature */
        /* TODO: feed RXSM status bits */
        /* TODO: feed subsystem status */

        encodeHeartbeatFrame(&heartbeat, reinterpret_cast<frames::heartbeat_frame_t*>(payload));
        send(frames::HEARTBEAT, payload, sizeof(frames::heartbeat_frame_t));

        osDelay(500);
    }
}
