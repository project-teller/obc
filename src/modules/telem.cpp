#include <cassert>
#include <cmsis_os2.h>
#include <cstring>

#include "hal/uart.h"
#include "modules/errors.h"
#include "modules/telem.h"
#include "utils/crc.h"

using namespace std;
using namespace teller::hal;
using namespace teller::telem;

typedef struct {
    /** Data to write to the UART */
    uint8_t* data;

    /** Length of data to write to the UART */
    uint16_t length;
} message_t;

static osMessageQueueId_t queue;

/** Number of chunks that can be enqueued in the task without blocking */
static const int QUEUE_SIZE = 64;

/** Maximum length of payload allowed in a telemetry message, inclusive */
static const int MAX_PAYLOAD_LENGTH = 63;

/** Sequence number of next message */
static uint8_t seq_no = 0;

static bool sendRaw(uint8_t* buf, uint16_t length);

bool teller::telem::init()
{
    queue = osMessageQueueNew(QUEUE_SIZE, sizeof(message_t), nullptr);
    return queue != nullptr;
}

void teller::telem::flushNext()
{
    osStatus_t result;
    message_t message;

    result = osMessageQueueGet(queue, &message, nullptr, osWaitForever);
    assert(result == osOK);

    if (message.data != nullptr) {
        uart::write(uart::TELEMETRY, message.data, message.length);
        free(message.data);
    }
}

bool teller::telem::send(const uint8_t* data, uint16_t length)
{
    uint8_t* buf = static_cast<uint8_t*>(malloc(length));
    TELLER_CHECK_OOM(buf);

    memcpy(buf, data, length);
    return sendRaw(buf, length);
}

bool teller::telem::send(const char* data)
{
    return send(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), strlen(data));
}

bool teller::telem::send(
    envelope_t envelope, const uint8_t* payload, uint16_t length)
{
    uint8_t* buf;
    uint16_t crc;

    if (length > MAX_PAYLOAD_LENGTH) {
        return false;
    }

    /*
     * The entire message should be formatted as follows:
     *
     * - Two sync bytes: 0xCA and 0xFE
     * - Sequence number (one byte)
     * - Frame type (one byte)
     * - Source and target component IDs in one byte (source in upper nibble,
     *   target in lower nibble)
     * - Payload length (one byte)
     * - Payload
     * - CRC-CCITT checksum
     *
     * The envelope therefore has an overhead of 8 bytes.
     */

    buf = static_cast<uint8_t*>(malloc(length + 8));
    TELLER_CHECK_OOM(buf);

    if (envelope.source == UNKNOWN_COMPONENT) {
        envelope.source = ONBOARD_COMPUTER;
    }

    if (envelope.target == UNKNOWN_COMPONENT) {
        envelope.target = GROUND_STATION;
    }

    buf[0] = 0xCA;
    buf[1] = 0xFE;
    buf[2] = seq_no++;
    buf[3] = envelope.frame_type;
    buf[4] = (((static_cast<int>(envelope.source) & 0x03) << 4) | (static_cast<int>(envelope.target) & 0x03));
    buf[5] = length;
    memcpy(buf + 6, payload, length);

    crc = crc_ccitt(0, buf, length + 6);
    buf[length + 6] = crc & 0xff;
    buf[length + 7] = crc >> 8;

    return sendRaw(buf, length + 8);
}

bool teller::telem::send(
    teller::telem::frames::frame_type_t type, const uint8_t* payload,
    uint16_t length)
{
    envelope_t envelope;
    envelope.frame_type = static_cast<uint8_t>(type);
    envelope.source = GROUND_STATION;
    envelope.target = GROUND_STATION;
    return send(envelope, payload, length);
}

/* ************************************************************************* */

bool sendRaw(uint8_t* buf, uint16_t length)
{
    message_t message;
    osStatus_t result;

    message.data = buf;
    message.length = length;

    result = osMessageQueuePut(queue, &message, 0, osWaitForever);
    assert(result == osOK);

    return true;
}
