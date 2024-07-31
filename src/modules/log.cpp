#include <cstring>

#include "core/telem/text_message.h"
#include "modules/imu.h"
#include "modules/log.h"
#include "modules/telem.h"

using namespace teller::telem;

namespace teller::log {

/**
 * @brief Type specification for functions that the logging module will call periodically.
 */
typedef void log_func_t(void);

typedef struct {
    uint16_t period; /**< Period multiplier for the log task */
    log_func_t* func; /**< Function to call when this task needs to be executed */
    uint16_t counter;
} task_t;

#define NO_MORE_TASKS \
    {                 \
        0             \
    }

/**
 * @brief Table containing all the logging tasks that the system needs to execute periodically.
 */
task_t tasks[] = {
    { 1, teller::imu::log },
    NO_MORE_TASKS
};

static uint8_t payload[MAX_PAYLOAD_LENGTH];
static Logger loggers[NUM_MODULES];

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

void runSingleIteration()
{
    for (task_t* task = tasks; task->period > 0; task++) {
        task->counter++;
        if (task->counter >= task->period) {
            task->counter = 0;
            task->func();
        }
    }
}

Logger* getLogger(teller::telem::module_id_t module)
{
    return &loggers[module];
}

bool sendToTelemetry(
    teller::telem::module_id_t module, teller::telem::log_level_t level,
    const char* message, uint32_t timeout)
{
    frames::text_message_data_t data;

    data.module = module;
    data.level = level;
    strncpy(data.message, message, teller::telem::frames::MAX_TEXT_MESSAGE_LENGTH);
    data.message[teller::telem::frames::MAX_TEXT_MESSAGE_LENGTH] = 0;

    teller::telem::send(
        frames::TEXT_MESSAGE, payload,
        frames::encodeTextMessageFrame(&data, payload),
        timeout);

    return true;
}

Logger::Logger(module_id_t module)
    : _module(module)
{
}

}
