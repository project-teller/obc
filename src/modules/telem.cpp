#include <cassert>
#include <cstring>

#include "core/log_records.h"
#include "core/utils/crc.h"
#include "hal/memory.h"
#include "hal/queue.hpp"
#include "hal/uart.h"
#include "modules/edr.hpp"
#include "modules/errors.h"
#include "modules/messages.h"
#include "modules/telem.h"

using namespace std;
using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

using teller::edr::FormattedLogRecord;

typedef struct {
    /** Bitmask indicating the UARTs to write the message to */
    uint8_t targets;

    /** Data to write to the UART */
    uint8_t* data;

    /** Length of data to write to the UART */
    uint8_t length;
} message_t;

/**
 * @brief Type specification for functions that the logging module will call periodically.
 */
typedef void telem_func_t(uint8_t*);

typedef struct {
    uint16_t period; /**< Period multiplier for the telemetry task */
    telem_func_t* func; /**< Function to call when this task needs to be executed */
    uint16_t counter;
} task_t;

/** Bitmask indicating which UARTs we are sending telemetry to */
static uint8_t telemetry_channel_mask = 0;

/** Sequence number of next message */
static uint8_t seq_no = 0;

/** Number of chunks that can be enqueued in the task without blocking */
static const int QUEUE_SIZE = 64;

/** Queue in which the enqueued messages are stored */
static BlockingQueue<message_t> out_queue(QUEUE_SIZE);

static void sendHeartbeat(uint8_t* payload);
static void sendClockStatus(uint8_t* payload);
static void sendIMUMeasurement(uint8_t* payload);

static bool sendLowLevel(uint8_t targets, uint8_t* buf, uint8_t length, uint32_t timeout);

#define NO_MORE_TASKS \
    {                 \
        0             \
    }

/**
 * @brief Table containing all the telemetry-related tasks that the system needs to execute periodically.
 *
 * Not all telemetry messages are handled here. For instance, IMU measurements
 * are sent from here because they are decoupled from the sampling loop in the
 * IMU module. However, GMM measurements are sent from the GMM subsystem because
 * they have to match the messages received from the GMM unambiguously.
 */
task_t tasks[] = {
    { 25, sendHeartbeat },
    { 250, sendClockStatus },
    { 1, sendIMUMeasurement },
    NO_MORE_TASKS
};

/** Log message format for board voltage and temperature */
static FormattedLogRecord<uint32_t, uint8_t, uint8_t> brdLogRecord(
    LOG_RECORD_BRD, "BRD", "TimeMS,Voltage,Temp", "IBb", "sOO", "CA0");

namespace teller::telem {

bool init()
{
    seq_no = 0;
    telemetry_channel_mask = 0;

    requestTelemetry(uart::TELEMETRY);

    return true;
}

void destroy()
{
    message_t message;

    while (!out_queue.empty()) {
        out_queue.receive(message);
        if (message.data != nullptr) {
            free(message.data);
        }
    }

    seq_no = 0;
    telemetry_channel_mask = 0;
}

BlockingQueueBase* getQueue()
{
    return &out_queue;
}

bool flushNext()
{
    message_t message;

    if (!out_queue.receive(message)) {
        return false;
    }

    if (message.data != nullptr) {
        if (message.targets & (1 << uart::TELEMETRY)) {
            uart::write(uart::TELEMETRY, message.data, message.length);
        }

        if (message.targets & (1 << uart::DEBUG)) {
            uart::write(uart::DEBUG, message.data, message.length);
        }

        free(message.data);
    }

    return true;
}

void requestTelemetry(uart::uart_t index)
{
    telemetry_channel_mask |= (1 << index);
}

void runSingleIteration(uint8_t* payload)
{
    for (task_t* task = tasks; task->period > 0; task++) {
        task->counter++;
        if (task->counter >= task->period) {
            task->counter = 0;
            task->func(payload);
        }
    }
}

bool send(
    teller::telem::frames::frame_type_t type, const uint8_t* payload,
    uint8_t length, uint32_t timeout)
{
    return sendTo(telemetry_channel_mask, type, payload, length, timeout);
}

bool send(envelope_t envelope, const uint8_t* payload, uint8_t length, uint32_t timeout)
{
    return sendTo(telemetry_channel_mask, envelope, payload, length, timeout);
}

bool sendTo(
    uint8_t targets, teller::telem::frames::frame_type_t type, const uint8_t* payload,
    uint8_t length, uint32_t timeout)
{
    envelope_t envelope;
    envelope.frame_type = static_cast<uint8_t>(type);
    envelope.source = ONBOARD_COMPUTER;
    envelope.target = GROUND_STATION;
    return sendTo(targets, envelope, payload, length, timeout);
}

bool sendTo(uint8_t targets, envelope_t envelope, const uint8_t* payload, uint8_t length, uint32_t timeout)
{
    uint8_t* buf;
    uint8_t buf_length;
    bool success = false;

    if (payload == nullptr) {
        length = 0;
    }

    buf_length = getMessageSizeForPayloadLength(length);
    if (buf_length == 0) {
        return false; /* LCOV_EXCL_LINE */
    }

    buf = static_cast<uint8_t*>(teller::hal::memory::malloc(buf_length));
    TELLER_CHECK_OOM(buf);

    envelope.seq_no = seq_no++;

    if (!serialize(buf, buf_length, envelope, payload, length)) {
        goto cleanup;
    }

    success = sendLowLevel(targets, buf, length + 8, timeout);

cleanup:
    if (!success) {
        teller::hal::memory::free(buf);
    }

    return success;
}

void stopTelemetry(uart::uart_t index)
{
    telemetry_channel_mask &= ~(1 << index);
}

}

/* ************************************************************************* */

bool sendLowLevel(uint8_t targets, uint8_t* buf, uint8_t length, uint32_t timeout)
{
    message_t message = {
        .targets = targets,
        .data = buf,
        .length = length
    };
    return out_queue.send_with_timeout(message, timeout);
}

/* ************************************************************************* */

using teller::telem::send;

static void sendHeartbeat(uint8_t* payload)
{
    frames::heartbeat_data_t heartbeat;

    memset(&heartbeat, 0, sizeof(heartbeat));
    updateHeartbeatData(&heartbeat);
    send(frames::HEARTBEAT, payload, encodeHeartbeatFrame(&heartbeat, payload));

    /*
    brdLogRecord.write(
        heartbeat.timestampInMsec,
        static_cast<uint8_t>(heartbeat.voltageInVolts * 0.1),
        static_cast<int8_t>(heartbeat.temperateInCelsius));
    */
}

static void sendClockStatus(uint8_t* payload)
{
    frames::clock_status_data_t clock_status;

    updateClockStatusData(&clock_status);
    send(frames::CLOCK_STATUS, payload, encodeClockStatusFrame(&clock_status, payload));
}

static void sendIMUMeasurement(uint8_t* payload)
{
    frames::imu_data_t imu_measurement;

    updateIMUMeasurement(&imu_measurement);
    send(frames::IMU, payload, encodeIMUFrame(&imu_measurement, payload));
}
