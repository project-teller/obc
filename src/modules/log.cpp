#include <cstring>

#include "core/telem/text_message.h"
#include "modules/log.h"
#include "modules/telem.h"

using namespace teller::telem;

namespace teller::log {

static uint8_t payload[MAX_PAYLOAD_LENGTH];
static Logger loggers[MODULE_COUNT];

bool init()
{
    size_t n = sizeof(loggers) / sizeof(loggers[0]);
    for (size_t i = 0; i < n; i++) {
        loggers[i]._module = static_cast<teller::telem::module_id_t>(i);
    }

    return true;
}

void destroy()
{
}

const Logger& getLogger(teller::telem::module_id_t module)
{
    return loggers[module];
}

bool send(
    teller::telem::module_id_t module, teller::telem::log_level_t level,
    const char* message)
{
    frames::text_message_data_t data;

    data.module = module;
    data.level = level;
    strncpy(data.message, message, teller::telem::frames::MAX_TEXT_MESSAGE_LENGTH);
    data.message[teller::telem::frames::MAX_TEXT_MESSAGE_LENGTH] = 0;

    teller::telem::send(
        frames::TEXT_MESSAGE, payload,
        frames::encodeTextMessageFrame(&data, payload));

    return true;
}

Logger::Logger(module_id_t module)
    : _module(module)
{
}

}
