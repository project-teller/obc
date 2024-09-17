#include <cstring>
#include <optional>

#include "core/telem/ack.h"
#include "core/telem/calibration.h"
#include "core/telem/parser.h"
#include "core/telem/storage.h"
#include "hal/system.h"
#include "modules/cmd.h"
#include "modules/edr.hpp"
#include "modules/imu.h"
#include "modules/log.h"
#include "modules/storage.h"
#include "modules/telem.h"

using namespace std;
using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;
using teller::hal::uart::uart_t;

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

static optional<Response> processPacket(uart_t channel, const envelope_t& envelope, const uint8_t* payload);
static optional<Response> processCalibrationPacket(const envelope_t& envelope, const uint8_t* payload);
static optional<Response> processStoragePacket(uart_t channel, const envelope_t& envelope, const uint8_t* payload);
static bool sendResponse(uart_t channel, const envelope_t& envelope, Response response, uint8_t* buf);

static Logger* logger;

/* TODO(ntamas): this is wasteful, we are allocating parsers also for those
 * UART channels where we will not have commands */
static Parser parsers[teller::hal::uart::NUM_UARTS];

namespace teller::cmd {

bool init()
{
    for (int i = 0; i < teller::hal::uart::NUM_UARTS; i++) {
        parsers[i].reset();
    }

    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
}

bool handleCommands(uart_t index, uint8_t* buf)
{
    uint8_t ch;
    optional<Response> response;
    Parser* parser;

    if (!uart::read1(index, &ch, uart::WAIT_FOREVER)) {
        return false;
    }

    parser = &parsers[index];

    if (parser->feed(ch)) {
        const envelope_t& envelope = parser->getEnvelope();
        if (envelope.target == ONBOARD_COMPUTER) {
            /* This is a packet for us */
            response = processPacket(index, envelope, parser->getPayload());
        } else {
            /* This is a packet for some other component */
            response.reset();
            /* TODO(ntamas): forward to SCM if needed */
        }

        if (response) {
            /* return value ignored, we can't do much if we can't send
             * responses */
            sendResponse(index, envelope, *response, buf);
        }

        return true;
    } else {
        return false;
    }
}

}

#define REJECT_UNLESS_FROM_GCS(what)                                       \
    if (envelope.source != GROUND_STATION) {                               \
        logger->warning("Ignored %s req from c%d", what, envelope.source); \
    } else

optional<Response> processPacket(uart_t channel, const envelope_t& envelope, const uint8_t* payload)
{
    switch (envelope.frame_type) {

    case frames::RESET:
        REJECT_UNLESS_FROM_GCS("reset")
        {
            /* TODO(ntamas): send ACK, delay the reset to leave some time for
             * the serial queue to flush */
            teller::hal::system::requestReset();
        }
        break;

    case frames::STORAGE:
        REJECT_UNLESS_FROM_GCS("storage")
        {
            return processStoragePacket(channel, envelope, payload);
        }
        break;

    case frames::CALIBRATION:
        REJECT_UNLESS_FROM_GCS("calibration")
        {
            return processCalibrationPacket(envelope, payload);
        }

    default:
        /* We are not interested in this packet */
        logger->warning("Unhandled pkt: %d", envelope.frame_type);
        break;
    }

    return {};
}

bool sendResponse(uart_t channel, const envelope_t& envelope, Response response, uint8_t* buf)
{
    frames::ack_data_t data = {
        .frame_type = static_cast<frames::frame_type_t>(envelope.frame_type),
        .seq_no = envelope.seq_no,
        .result = response.result,
        .error = response.error,
        .value = response.value
    };
    uint8_t length = frames::encodeAckFrame(&data, buf);
    return teller::telem::sendTo((1 << channel), frames::ACK, buf, length);
}

optional<Response> processCalibrationPacket(const envelope_t& envelope, const uint8_t* payload)
{
    frames::calibration_request_data_t data;
    int retval = 0;

    /* TODO(ntamas): check the length of the packet! */
    frames::decodeCalibrationRequestFrame(payload, &data);

    switch (data.procedure) {
    case frames::CALIBRATION_NOP:
        break;

    case frames::CALIBRATION_GYRO:
        teller::imu::startGyroCalibration();
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

optional<Response> processStoragePacket(uart_t channel, const envelope_t& envelope, const uint8_t* payload)
{
    frames::storage_command_data_t data;
    int retval;

    /* TODO(ntamas): check the length of the packet! */
    frames::decodeStorageCommandFrame(payload, &data);

    switch (data.command) {
    case frames::STORAGE_COMMAND_MOUNT:
        retval = teller::storage::mountStorage(data.area);
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
        retval = teller::storage::startReadingStorage(
            data.area, data.offset, data.length, (1 << channel), envelope.seq_no);
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
