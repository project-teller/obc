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
    uint8_t ch;
    uint16_t bytes_read;
    Parser* parser;

    /* uart::readInto() is guaranteed to yield if there are other tasks with
     * higher or equal priorities */
    if (!uart::readInto(index, &ch, 1, &bytes_read)) {
        return false;
    }

    parser = &parsers[index];
    ch = parser->feed(ch);
    if (ch) {
        /* nonblocking mode used so we can return to reading the UART as fast
         * as possible */
        teller::cmd::feedNonblocking(index, parser->getEnvelope(), parser->getPayload(), ch - 1);
        return true;
    } else {
        return false;
    }
}

}
