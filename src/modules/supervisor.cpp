#include "modules/supervisor.h"
#include "hal/system.h"
#include "hal/watchdog.h"
#include "modules/log.h"
#include <cstring>

using namespace teller::hal::system;
using namespace teller::telem;

static teller::log::Logger* logger = nullptr;

#define MAX_TASKS 32
#define INVALID_TOKEN std::numeric_limits<uint8_t>::max()
#define UNLIMITED_NUDGES std::numeric_limits<uint16_t>::max()

/**
 * @brief Structure in which the status and configuration of a registered task is stored.
 */
typedef struct {
    /** Human-readable name of the task; null if the slot is unused */
    const char* name;

    /** Number of seconds between consecutive checks of the task counters */
    uint8_t interval_sec;

    /** Minimum number of nudges to receive from the task between consecutive checks */
    uint16_t min_nudges;

    /**
     * Maximum number of nudges to receive from the task between consecutive checks;
     * 0xFFFF when unlimited.
     */
    uint16_t max_nudges;

    /**
     * Counter storing the number of nudges received from the task since the
     * last check.
     */
    uint16_t nudges;

    /** Counter storing the number of iterations remaining until the next check */
    uint8_t time_until_next_check_sec;
} task_stats_t;

static_assert(MAX_TASKS < INVALID_TOKEN);
static_assert(MAX_TASKS <= 32);
static task_stats_t task_stats[MAX_TASKS];

static task_stats_t* getTaskFromToken(teller::supervisor::task_token_t token);
static bool isValidTask(const task_stats_t* stat);
static teller::supervisor::task_token_t registerTask(const char* name);
static void unregisterTask(teller::supervisor::task_token_t token);
static void nudgeTask(teller::supervisor::task_token_t token);

namespace teller::supervisor {

TaskRegistration::TaskRegistration(const char* name)
    : m_token(INVALID_TOKEN)
{
    m_token = registerTask(name);
}

TaskRegistration::~TaskRegistration()
{
    unregisterTask(m_token);
}

void TaskRegistration::nudge()
{
    nudgeTask(m_token);
}

TaskRegistration& TaskRegistration::inSeconds(uint8_t interval_sec)
{
    task_stats_t* task = getTaskFromToken(m_token);
    if (task && interval_sec >= 1) {
        task->interval_sec = interval_sec;
        if (task->time_until_next_check_sec > task->interval_sec) {
            task->time_until_next_check_sec = task->interval_sec;
        }
    }
    return (*this);
}

TaskRegistration& TaskRegistration::expect(uint16_t min, uint16_t max)
{
    task_stats_t* task = getTaskFromToken(m_token);
    if (task) {
        task->min_nudges = min;
        task->max_nudges = max;
    }
    return (*this);
}

bool init()
{
    logger = teller::log::getLogger(MODULE_ID_OBC);
    if (!logger) {
        return false;
    }

    memset(task_stats, 0, sizeof(task_stats));

    return true;
}

void destroy()
{
    memset(task_stats, 0, sizeof(task_stats));
    logger = nullptr;
}

void setup()
{
    teller::hal::watchdog::configureAndStart();
    logger->info("TELLER OBC booted");
}

void checkTasks(uint32_t timestamp)
{
    uint8_t i;
    task_stats_t* task;

    if (timestamp == 0) {
        timestamp = getTimeSinceBootMsec();
    }

    for (i = 0; i < MAX_TASKS; i++) {
        task = &task_stats[i];
        if (!isValidTask(task)) {
            continue;
        }

        task->time_until_next_check_sec--;
        if (task->time_until_next_check_sec) {
            continue;
        }

        if (task->nudges == 0 && task->min_nudges > 0) {
            logger->error_nowait("%s: task stalled", task->name);
        } else if (task->nudges < task->min_nudges) {
            logger->warning_nowait(
                "%s: task too slow (%d/%d)", task->name,
                task->nudges, task->min_nudges);
        } else if (task->max_nudges < UNLIMITED_NUDGES && task->nudges > task->max_nudges) {
            logger->warning_nowait(
                "%s: task too fast (%d/%d)", task->name,
                task->nudges, task->max_nudges);
        }

        task->time_until_next_check_sec = task->interval_sec;
        task->nudges = 0;
    }

    teller::hal::watchdog::reset();
}

}

static task_stats_t* getTaskFromToken(teller::supervisor::task_token_t token)
{
    if (token >= 0 && token < MAX_TASKS) {
        return &task_stats[token];
    } else {
        return nullptr;
    }
}

static bool isValidTask(const task_stats_t* stat)
{
    return stat->name != nullptr && stat->interval_sec > 0;
}

static teller::supervisor::task_token_t registerTask(const char* name)
{
    task_stats_t* task;

    if (!name) {
        return INVALID_TOKEN;
    }

    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        task = &task_stats[i];
        if (!isValidTask(task)) {
            task->name = name;
            task->interval_sec = 1;
            task->time_until_next_check_sec = 1;
            task->min_nudges = 0;
            task->max_nudges = std::numeric_limits<uint16_t>::max();
            return static_cast<teller::supervisor::task_token_t>(i);
        }
    }

    return INVALID_TOKEN;
}

static void unregisterTask(teller::supervisor::task_token_t token)
{
    task_stats_t* task = getTaskFromToken(token);
    if (task) {
        memset(task, 0, sizeof(task_stats_t));
    }
}

static void nudgeTask(teller::supervisor::task_token_t token)
{
    task_stats_t* task = getTaskFromToken(token);
    if (task) {
        task->nudges++;
    }
}
