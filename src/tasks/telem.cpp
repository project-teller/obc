#include <cstring>

#include "modules/messages.h"
#include "modules/telem.h"
#include "tasks/serial.h"
#include "tasks/telem.h"

using namespace teller::telem;

const osThreadAttr_t teller::tasks::telemetryTaskAttr = {
    .name = "telem",
    .stack_size = 1024,
    .priority = osPriorityNormal,
};

__NO_RETURN void teller::tasks::telemetryTask(void* arg)
{
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    uint8_t to_send;
    uint8_t loop_counter;
    frames::heartbeat_data_t heartbeat;
    frames::timesync_data_t timesync;

    memset(&heartbeat, 0, sizeof(heartbeat));

    for (loop_counter = 0;; loop_counter++) {
        /* Heartbeat is sent in every iteration, i.e. twice per second */
        updateHeartbeatData(&heartbeat);
        to_send = encodeHeartbeatFrame(&heartbeat, payload);
        send(frames::HEARTBEAT, payload, to_send);

        /* Timesync packet is sent once every 8 iterations, i.e. at 4 Hz */
        if (loop_counter % 8 == 0) {
        }

        osDelay(500);
    }
}
