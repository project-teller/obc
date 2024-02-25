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

static void sendHeartbeat(uint8_t* payload);
static void sendTimesync(uint8_t* payload);

__NO_RETURN void teller::tasks::telemetryTask(void* arg)
{
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    uint8_t loop_counter;

    for (loop_counter = 0;; loop_counter++) {
        /* Heartbeat is sent in every iteration, i.e. twice per second */
        sendHeartbeat(payload);

        /* Timesync packet is sent once every 8 iterations, i.e. at 4 Hz */
        if (loop_counter % 8 == 0) {
            sendTimesync(payload);
        }

        osDelay(500);
    }
}

static void sendHeartbeat(uint8_t* payload)
{
    frames::heartbeat_data_t heartbeat;

    memset(&heartbeat, 0, sizeof(heartbeat));
    updateHeartbeatData(&heartbeat);
    send(frames::HEARTBEAT, payload, encodeHeartbeatFrame(&heartbeat, payload));
}

static void sendTimesync(uint8_t* payload)
{
    frames::timesync_data_t timesync;

    updateTimesyncData(&timesync);
    send(frames::TIMESYNC, payload, encodeTimesyncFrame(&timesync, payload));
}
