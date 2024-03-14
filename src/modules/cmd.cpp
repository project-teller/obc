#include <cstdio>
#include <cstring>

#include "core/telem/parser.h"
#include "hal/system.h"
#include "hal/uart.h"
#include "modules/log.h"
#include "tasks/cmd.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::telem;

static void processPacket(const envelope_t& envelope, const uint8_t* payload);

static Logger* logger;
static Parser parser;

namespace teller::cmd {

bool init()
{
    parser.reset();
    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
}

bool handleNext()
{
    uint8_t ch;
    uint16_t read;

    if (!uart::read(uart::TELEMETRY, &ch, 1, &read) || read <= 0) {
        return false;
    }

    if (parser.feed(ch)) {
        const envelope_t& envelope = parser.getEnvelope();
        if (envelope.target == ONBOARD_COMPUTER) {
            /* This is a packet for us */
            processPacket(envelope, parser.getPayload());
        } else {
            /* This is a packet for some other component */
            /* TODO(ntamas): forward to SCM if needed */
        }

        return true;
    } else {
        return false;
    }
}

}

void processPacket(const envelope_t& envelope, const uint8_t* payload)
{
    switch (envelope.frame_type) {

    case frames::RESET:
        /* Reset requested. The sender must be the ground station. */
        if (envelope.source == GROUND_STATION) {
            /* TODO(ntamas): send ACK, delay the reset to leave some time for
             * the serial queue to flush */
            teller::hal::system::requestReset();
        } else {
            logger->warning("Ignored reset req from c%d", envelope.source);
        }
        break;

    default:
        /* We are not interested in this packet */
        logger->warning("Unhandled pkt: %d", envelope.frame_type);
        break;
    }
}
