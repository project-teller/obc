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
    frames::heartbeat_data_t heartbeat;

    memset(&heartbeat, 0, sizeof(heartbeat));

    for (;;) {
        updateHeartbeatData(&heartbeat);
        to_send = encodeHeartbeatFrame(&heartbeat, payload);
        send(frames::HEARTBEAT, payload, to_send);

        osDelay(500);
    }
}
