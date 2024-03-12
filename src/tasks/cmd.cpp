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

[[noreturn]] void teller::tasks::commandTask(void* arg)
{
    uint8_t ch;
    uint16_t read;
    Parser parser;

    while (true) {
        if (uart::read(uart::TELEMETRY, &ch, 1, &read) && read > 0) {
            if (parser.feed(ch)) {
                const envelope_t& envelope = parser.getEnvelope();
                if (envelope.target == ONBOARD_COMPUTER) {
                    /* This is a packet for us */
                    processPacket(envelope, parser.getPayload());
                } else {
                    /* This is a packet for some other component */
                    /* TODO(ntamas): forward to SCM if needed */
                }
            }
        }
    }
}

void processPacket(const envelope_t& envelope, const uint8_t* payload)
{
    switch (envelope.frame_type) {
    default:
        /* We are not interested in this packet */
        getLogger(MODULE_ID_OBC).warning("Unhandled packet type: %d", envelope.frame_type);
        break;
    }
}
