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
using teller::hal::uart::NUM_UARTS;
using teller::hal::uart::uart_t;

static Logger* logger;

/* TODO(ntamas): this is wasteful, we are allocating parsers also for those
 * UART channels where we will not have commands */
static Parser parsers[NUM_UARTS];

static uint8_t rx_buf[256];

namespace teller::uart_rx {

bool init()
{
    for (int i = 0; i < NUM_UARTS; i++) {
        parsers[i].reset();
    }

    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
}

bool read(uart_t index)
{
    uint16_t i, bytes_read;
    Parser* parser;
    uint8_t payload_length_plus_one;
    bool result = false;

    /* uart::read() is guaranteed to yield if there are other tasks with
     * higher or equal priorities */
    if (uart::read(index, rx_buf, sizeof(rx_buf), &bytes_read)) {
        for (i = 0; i < bytes_read; i++) {
            parser = &parsers[index];
            payload_length_plus_one = parser->feed(rx_buf[i]);
            if (payload_length_plus_one) {
                /* nonblocking mode used so we can return to reading the UART as fast
                 * as possible */
                teller::cmd::feedNonblocking(
                    index, parser->getEnvelope(), parser->getPayload(),
                    payload_length_plus_one - 1);
                result = true;
            }
        }
    }

    return result;
}

}
