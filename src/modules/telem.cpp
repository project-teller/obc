#include <cassert>
#include <cmsis_os2.h>
#include <cstring>

#include "core/utils/crc.h"
#include "hal/uart.h"
#include "modules/errors.h"
#include "modules/telem.h"

using namespace std;
using namespace teller::hal;
using namespace teller::telem;

typedef struct {
    /** Data to write to the UART */
    uint8_t* data;

    /** Length of data to write to the UART */
    uint8_t length;
} message_t;

/** Sequence number of next message */
static uint8_t seq_no = 0;

static bool initLowLevel();
static bool receiveLowLevel(message_t* message);
static bool sendLowLevel(uint8_t* buf, uint8_t length);

bool teller::telem::init()
{
    return initLowLevel();
}

bool teller::telem::flushNext()
{
    message_t message;

    if (!receiveLowLevel(&message)) {
        return false;
    }

    if (message.data != nullptr) {
        uart::write(uart::TELEMETRY, message.data, message.length);
        free(message.data);
    }

    return true;
}

bool teller::telem::send(const uint8_t* data, uint8_t length)
{
    uint8_t* buf = static_cast<uint8_t*>(malloc(length));
    TELLER_CHECK_OOM(buf);

    memcpy(buf, data, length);
    return sendLowLevel(buf, length);
}

bool teller::telem::send(const char* data)
{
    return send(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), strlen(data));
}

bool teller::telem::send(
    envelope_t envelope, const uint8_t* payload, uint8_t length)
{
    uint8_t* buf;
    uint8_t buf_length = getMessageSizeForPayloadLength(length);

    if (buf_length == 0) {
        return false;
    }

    buf = static_cast<uint8_t*>(malloc(buf_length));
    TELLER_CHECK_OOM(buf);

    envelope.seq_no = seq_no++;

    if (!serialize(buf, buf_length, envelope, payload, length)) {
        return false;
    }

    return sendLowLevel(buf, length + 8);
}

bool teller::telem::send(
    teller::telem::frames::frame_type_t type, const uint8_t* payload,
    uint8_t length)
{
    envelope_t envelope;
    envelope.frame_type = static_cast<uint8_t>(type);
    envelope.source = ONBOARD_COMPUTER;
    envelope.target = GROUND_STATION;
    return send(envelope, payload, length);
}

/* ************************************************************************* */
/* CMSIS-specific implementation of the telemetry module                     */
/* ************************************************************************* */

static osMessageQueueId_t queue;

/** Number of chunks that can be enqueued in the task without blocking */
static const int QUEUE_SIZE = 64;

bool initLowLevel()
{
    queue = osMessageQueueNew(QUEUE_SIZE, sizeof(message_t), nullptr);
    return queue != nullptr;
}

bool receiveLowLevel(message_t* message)
{
    return osMessageQueueGet(queue, message, nullptr, osWaitForever) == osOK;
}

bool sendLowLevel(uint8_t* buf, uint8_t length)
{
    message_t message = {
        .data = buf,
        .length = length
    };
    return osMessageQueuePut(queue, &message, 0, osWaitForever) == osOK;
}
