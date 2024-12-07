#include <cstring>
#include <optional>

#include "core/telem/ack.h"
#include "core/telem/calibration.h"
#include "core/telem/clock_sync.h"
#include "core/telem/debug.h"
#include "core/telem/echo.h"
#include "core/telem/lcl_reset.h"
#include "core/telem/parser.h"
#include "core/telem/storage.h"
#include "hal/memory.h"
#include "hal/rtc.h"
#include "hal/system.h"
#include "modules/cam.h"
#include "modules/cmd.h"
#include "modules/edr.hpp"
#include "modules/imu.h"
#include "modules/lcl.h"
#include "modules/log.h"
#include "modules/scheduler.h"
#include "modules/storage.h"
#include "modules/telem.h"

using namespace std;
using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;
using teller::hal::uart::uart_t;

typedef struct {
    /** Index of the UART that the message was received from */
    uart_t index;

    /** Envelope of the message */
    envelope_t envelope;

    /** Pointer to the payload of the message */
    uint8_t* payload;

    /** Length of the payload */
    uint8_t length;
} InboundMessage;

/**
 * @brief Description of the response to be posted for an incoming packet.
 *
 * Any error response with an error code equal to zero will be ignored and no
 * response will be sent.
 */
class Response {
public:
    frames::ack_result_t result;
    int error;
    uint32_t value;

    Response(frames::ack_result_t _result = frames::ACK_ACCEPTED, int _error = 0, uint32_t _value = 0)
        : result(_result)
        , error(_error)
        , value(_value)
    {
    }

    static Response ok(uint32_t _value = 0)
    {
        return Response(frames::ACK_ACCEPTED, 0, _value);
    }

    static Response denied(int _error = 0)
    {
        return Response(frames::NAK_DENIED, _error);
    }

    static Response failed(int _error = 0)
    {
        return Response(frames::NAK_FAILED, _error);
    }

    static Response invalid(int _error = 0)
    {
        return Response(frames::NAK_INVALID, _error);
    }

    static Response unsupported(int _error = 0)
    {
        return Response(frames::NAK_UNSUPPORTED, _error);
    }
};

/** Number of messages that can be waiting to be processed in the task without blocking */
static const int QUEUE_SIZE = 64;

/** Queue in which the incoming messages are stored */
static BlockingQueue<InboundMessage> in_queue(QUEUE_SIZE);

static bool prepareMessage(InboundMessage& message, uart_t index,
    const envelope_t& envelope, const uint8_t* payload, uint8_t length);
static optional<Response> processPacket(const InboundMessage& message);
static optional<Response> processCalibrationPacket(const InboundMessage& message);
static optional<Response> processClockSyncPacket(const InboundMessage& message);
static optional<Response> processDebugPacket(const InboundMessage& message);
static bool processEchoPacket(const InboundMessage& message);
static optional<Response> processLCLResetRequestPacket(const InboundMessage& message);
static optional<Response> processStoragePacket(const InboundMessage& message);
static bool sendResponse(uart_t channel, const envelope_t& envelope, Response response);

static Logger* logger;
static uint8_t responseBuffer[MAX_PAYLOAD_LENGTH];

namespace teller::cmd {

bool init()
{
    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;
}

void destroy()
{
    InboundMessage message;

    while (!in_queue.empty()) {
        in_queue.receive(message);
        if (message.payload != nullptr) {
            teller::hal::memory::free(message.payload);
        }
    }

    logger = nullptr;
}

teller::hal::BlockingQueueBase* getQueue(void)
{
    return &in_queue;
}

void feed(
    teller::hal::uart::uart_t index,
    const teller::telem::envelope_t& envelope,
    const std::uint8_t* payload, std::uint8_t length)
{
    InboundMessage message;
    if (prepareMessage(message, index, envelope, payload, length)) {
        in_queue.send(message);
    }
}

bool feedNonblocking(
    teller::hal::uart::uart_t index,
    const teller::telem::envelope_t& envelope,
    const std::uint8_t* payload, std::uint8_t length)
{
    InboundMessage message;
    return (
        prepareMessage(message, index, envelope, payload, length) && in_queue.send_or_drop(message));
}

bool processNext(void)
{
    InboundMessage message;
    optional<Response> response;

    if (!in_queue.receive(message)) {
        return false;
    }

    if (message.payload != nullptr) {
        if (message.envelope.target == ONBOARD_COMPUTER) {
            /* This is a packet for us */
            response = processPacket(message);
        } else {
            /* This is a packet for some other component */
            response.reset();
        }

        teller::hal::memory::free(message.payload);

        if (response) {
            /* return value ignored, we can't do much if we can't send
             * responses */
            sendResponse(message.index, message.envelope, *response);
        }
    }

    return true;
}

bool performCalibration(teller::telem::frames::calibration_procedure_t procedure)
{
    switch (procedure) {
    case frames::CALIBRATION_NOP:
        return true;

    case frames::CALIBRATION_GYRO:
        teller::imu::startGyroCalibration();
        return true;

    case frames::CALIBRATION_ACCEL:
        return false;

    default:
        return false;
    }
}

size_t waiting(void)
{
    return in_queue.size();
}
}

bool prepareMessage(InboundMessage& message, uart_t index,
    const envelope_t& envelope, const uint8_t* payload, uint8_t length)
{
    std::uint8_t* payloadCopy = static_cast<std::uint8_t*>(teller::hal::memory::malloc(length > 0 ? length : 1));
    if (!payloadCopy) {
        return false;
    }

    memcpy(payloadCopy, payload, length);

    message.index = index;
    message.envelope = envelope;
    message.payload = payloadCopy;
    message.length = length;

    return true;
}

#define IGNORE_UNLESS_FROM_GCS(what)                                       \
    if (envelope.source != GROUND_STATION) {                               \
        logger->warning("Ignored %s req from c%d", what, envelope.source); \
    } else

optional<Response> processPacket(const InboundMessage& message)
{
    const envelope_t& envelope = message.envelope;

    switch (envelope.frame_type) {

    case frames::CLOCK_SYNC:
        IGNORE_UNLESS_FROM_GCS("clock sync")
        {
            return processClockSyncPacket(message);
        }
        break;

    case frames::RESET:
        IGNORE_UNLESS_FROM_GCS("reset")
        {
            /* TODO(ntamas): send ACK, delay the reset to leave some time for
             * the serial queue to flush */
            teller::hal::system::requestReset();
        }
        break;

    case frames::STORAGE:
        IGNORE_UNLESS_FROM_GCS("storage")
        {
            return processStoragePacket(message);
        }
        break;

    case frames::LCL_RESET:
        IGNORE_UNLESS_FROM_GCS("LCL reset")
        {
            return processLCLResetRequestPacket(message);
        }

    case frames::CALIBRATION:
        IGNORE_UNLESS_FROM_GCS("calibration")
        {
            return processCalibrationPacket(message);
        }

    case frames::ECHO:
        processEchoPacket(message);
        break;

    case frames::DEBUG:
        IGNORE_UNLESS_FROM_GCS("debug")
        {
            return processDebugPacket(message);
        }

    default:
        /* We are not interested in this packet */
        logger->warning("Unhandled pkt: %d", envelope.frame_type);
        break;
    }

    return {};
}

bool sendResponse(uart_t channel, const envelope_t& envelope, Response response)
{
    frames::ack_data_t data = {
        .frame_type = static_cast<frames::frame_type_t>(envelope.frame_type),
        .seq_no = envelope.seq_no,
        .result = response.result,
        .error = response.error,
        .value = response.value
    };
    uint8_t length = frames::encodeAckFrame(&data, responseBuffer);
    return teller::telem::sendTo((1 << channel), frames::ACK, responseBuffer, length);
}

optional<Response> processClockSyncPacket(const InboundMessage& message)
{
    frames::clock_sync_data_t data;

    if (!frames::validateEncodedClockSyncFrame(message.payload, message.length)) {
        return Response::invalid();
    }

    frames::decodeClockSyncFrame(message.payload, &data);

    if (teller::hal::rtc::setTimeMsec(data.timestampInMsec)) {
        teller::telem::sendClockStatusSoon();
        return Response::ok();
    } else {
        return Response::failed();
    }
}

optional<Response> processLCLResetRequestPacket(const InboundMessage& message)
{
    frames::lcl_reset_request_data_t data;

    if (!frames::validateEncodedLCLResetRequestFrame(message.payload, message.length)) {
        return Response::invalid();
    }

    frames::decodeLCLResetRequestFrame(message.payload, &data);
    teller::lcl::resetMultiple(data.lcls_to_reset);

    return Response::ok();
}

optional<Response> processCalibrationPacket(const InboundMessage& message)
{
    frames::calibration_request_data_t data;
    int retval = 0;

    if (!frames::validateEncodedCalibrationRequestFrame(message.payload, message.length)) {
        return Response::invalid();
    }

    frames::decodeCalibrationRequestFrame(message.payload, &data);

    switch (data.procedure) {
    case frames::CALIBRATION_NOP:
    case frames::CALIBRATION_GYRO:
        if (!teller::cmd::performCalibration(data.procedure)) {
            retval = 1;
        }
        break;

    case frames::CALIBRATION_ACCEL:
        return Response::unsupported();

    default:
        return Response::invalid();
    }

    if (retval) {
        return Response::failed(retval);
    } else {
        return Response::ok();
    }
}

optional<Response> processDebugPacket(const InboundMessage& message)
{
    frames::debug_command_data_t data;
    int retval = 0;

    if (!frames::validateEncodedDebugCommandFrame(message.payload, message.length)) {
        return Response::invalid();
    }

    frames::decodeDebugCommandFrame(message.payload, &data);

    switch (data.command) {
    case frames::DEBUG_CMD_NOP:
        break;

    case frames::DEBUG_CMD_START_CLOCK:
        teller::scheduler::start();
        teller::telem::sendClockStatusSoon();
        break;

    case frames::DEBUG_CMD_STOP_CLOCK:
        teller::scheduler::stop();
        teller::telem::sendClockStatusSoon();
        break;

    case frames::DEBUG_CMD_RESET_CLOCK:
        teller::scheduler::reset();
        teller::telem::sendClockStatusSoon();
        break;

    case frames::DEBUG_CMD_TOGGLE_CAMERA:
        teller::cam::sendPulse();
        break;

    case frames::DEBUG_CMD_TRIGGER_WATCHDOG:
        for (;;)
            ;
        break;

    case frames::DEBUG_CMD_REPORT_STORAGE_STATUS:
        teller::storage::reportStatus();
        break;

    default:
        return Response::invalid();
    }

    if (retval) {
        return Response::failed(retval);
    } else {
        return Response::ok();
    }
}

bool processEchoPacket(const InboundMessage& message)
{
    frames::echo_data_t data;
    uint8_t length;
    envelope_t envelope;

    if (!frames::validateEncodedEchoFrame(message.payload, message.length)) {
        return false;
    }

    frames::decodeEchoFrame(message.payload, message.length, &data);
    if (data.isReply) {
        return true;
    }

    data.isReply = 1;
    length = frames::encodeEchoFrame(&data, responseBuffer);

    envelope.frame_type = message.envelope.frame_type;
    envelope.source = ONBOARD_COMPUTER;
    envelope.target = message.envelope.source;

    return teller::telem::sendTo((1 << message.index), envelope, responseBuffer, length);
}

optional<Response> processStoragePacket(const InboundMessage& message)
{
    frames::storage_command_data_t data;
    int retval;

    if (!frames::validateEncodedStorageCommandFrame(message.payload, message.length)) {
        return Response::invalid();
    }

    frames::decodeStorageCommandFrame(message.payload, &data);

    switch (data.command) {
    case frames::STORAGE_COMMAND_MOUNT:
        retval = teller::storage::mountStorage(data.area, /* force = */ true);
        break;

    case frames::STORAGE_COMMAND_UNMOUNT:
        /* We cannot unmount the storage directly because there are
         * ExperimentDataRecorder instances logging into the area; we need to
         * ask the EDR to stop recording first, which will in turn unmount
         * the storage in the EDR task */
        retval = teller::edr::requestStopAndUnmount(data.area) ? 0 : EIO;
        break;

    case frames::STORAGE_COMMAND_ERASE:
        retval = teller::storage::eraseStorage(data.area);
        break;

    case frames::STORAGE_COMMAND_NOP:
        retval = 0;
        break;

    case frames::STORAGE_COMMAND_READ:
        /* TODO(ntamas): there's a deficiency in the protocol here; our commands
         * can contain a 32-bit address only, and large SD cards need 64-bit
         * addresses. However, we assume that you probably do not want to
         * download the raw image of an SD card over the telemetry channel */
        retval = teller::storage::startReadingStorage(
            data.area, data.offset, data.length, (1 << message.index),
            message.envelope.seq_no);
        break;

    case frames::STORAGE_COMMAND_GET_SIZE:
        return Response::ok(teller::storage::getStorageSize(data.area));

    case frames::STORAGE_COMMAND_GET_STATUS:
        return Response::ok(
            /* clang-format off */
            (teller::storage::isStorageConfigured(data.area) ? 1 : 0) |
            (teller::storage::isStorageMounted(data.area) ? 2 : 0) |
            (teller::storage::isStorageErrored(data.area) ? 4 : 0)
            /* clang-format on */
        );

    default:
        return Response::invalid();
    }

    if (retval) {
        return Response::failed(retval);
    } else {
        return Response::ok();
    }
}
