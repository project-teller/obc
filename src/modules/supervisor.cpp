#include "modules/supervisor.h"
#include "hal/mutex.hpp"
#include "hal/queue.hpp"
#include "hal/system.h"
#include "hal/watchdog.h"
#include "modules/log.h"
#include <cstring>

using teller::hal::lock_guard;
using teller::hal::mutex;

using namespace teller::hal::system;
using namespace teller::telem;
using teller::hal::BlockingQueueBase;

static teller::log::Logger* logger = nullptr;

#define MAX_TASKS 32
#define MAX_QUEUES 8
#define INVALID_TOKEN std::numeric_limits<uint8_t>::max()
#define UNLIMITED_NUDGES std::numeric_limits<uint16_t>::max()

/**
 * @brief Mutex protecting the task and queue stats structures during the registration phase.
 */
mutex stats_mutex;

/**
 * @brief Structure in which the status and configuration of a registered task is stored.
 */
typedef struct {
    /** Human-readable name of the task; null if the slot is unused */
    const char* name;

    /** Whether the task is enabled */
    bool enabled;

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

/**
 * @brief Structure in which the status and configuration of a registered queue is stored.
 */
typedef struct {
    /** Human-readable name of the queue; null if the slot is unused */
    const char* name;

    /** Pointer to the queue; null if the slot is unused */
    BlockingQueueBase* queue;

    /** High water mark: maximum number of items that we have seen in the queue */
    size_t high_water_mark;
} queue_stats_t;

static_assert(MAX_TASKS < INVALID_TOKEN);
static_assert(MAX_TASKS <= 32);
static task_stats_t task_stats[MAX_TASKS];

static_assert(MAX_QUEUES < INVALID_TOKEN);
static_assert(MAX_QUEUES <= 32);
static queue_stats_t queue_stats[MAX_QUEUES];

static void disableTask(teller::supervisor::task_token_t token);
static task_stats_t* getTaskFromToken(teller::supervisor::task_token_t token);
static bool isTaskEnabled(const task_stats_t* stats);
static bool isTaskValid(const task_stats_t* stat);
static teller::supervisor::task_token_t registerTask(const char* name);
static void unregisterTask(teller::supervisor::task_token_t token);
static void nudgeTask(teller::supervisor::task_token_t token);

static bool isQueueValid(const queue_stats_t* stat);
static queue_stats_t* getQueueFromToken(teller::supervisor::queue_token_t token);
static teller::supervisor::queue_token_t registerQueue(const char* name, BlockingQueueBase* queue);
static void unregisterQueue(teller::supervisor::queue_token_t token);

namespace teller::supervisor {

/* ************************************************************************** */

TaskRegistration::TaskRegistration(const char* name)
    : m_token(INVALID_TOKEN)
{
    m_token = registerTask(name);
}

TaskRegistration::~TaskRegistration()
{
    unregisterTask(m_token);
}

void TaskRegistration::disable()
{
    disableTask(m_token);
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
#ifdef TELLER_BOARD_POSIX
        /* This is not a realtime OS so expect deviations from the desired counts */
        task->min_nudges = min * 0.8;
#else
        task->min_nudges = min;
#endif
        task->max_nudges = max;
    }
    return (*this);
}

/* ************************************************************************** */

QueueRegistration::QueueRegistration(const char* name, BlockingQueueBase* queue)
    : m_token(INVALID_TOKEN)
{
    m_token = registerQueue(name, queue);
}

QueueRegistration::~QueueRegistration()
{
    unregisterQueue(m_token);
}

/* ************************************************************************** */

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

bool checkQueues(void)
{
    uint8_t i;
    uint8_t size;
    queue_stats_t* stats;
    bool result = true;

    for (i = 0; i < MAX_QUEUES; i++) {
        stats = &queue_stats[i];
        if (!isQueueValid(stats) || !stats->queue) {
            continue;
        }

        size = stats->queue->size();
        if (size > stats->high_water_mark) {
            stats->high_water_mark = size;
        }

        if (size >= stats->queue->limit()) {
            logger->error_nowait("%s: queue full", stats->name);
            result = false;
        }
    }

    return result;
}

bool checkTasks(uint32_t timestamp)
{
    uint8_t i;
    task_stats_t* stats;
    bool result = true;

    if (timestamp == 0) {
        timestamp = getTimeSinceBootMsec();
    }

    for (i = 0; i < MAX_TASKS; i++) {
        stats = &task_stats[i];
        if (!isTaskValid(stats) || !isTaskEnabled(stats)) {
            continue;
        }

        stats->time_until_next_check_sec--;
        if (stats->time_until_next_check_sec) {
            continue;
        }

        if (stats->nudges == 0 && stats->min_nudges > 0) {
            logger->error_nowait("%s: task stalled", stats->name);
            result = false;
        } else if (stats->nudges < stats->min_nudges) {
            logger->warning_nowait(
                "%s: task too slow (%d/%d)", stats->name,
                stats->nudges, stats->min_nudges);
            result = false;
        } else if (stats->max_nudges < UNLIMITED_NUDGES && stats->nudges > stats->max_nudges) {
            logger->warning_nowait(
                "%s: task too fast (%d/%d)", stats->name,
                stats->nudges, stats->max_nudges);
            result = false;
        }

        stats->time_until_next_check_sec = stats->interval_sec;
        stats->nudges = 0;
    }

    teller::hal::watchdog::reset();

    return result;
}

}

/* ************************************************************************** */

static task_stats_t* getTaskFromToken(teller::supervisor::task_token_t token)
{
    if (token >= 0 && token < MAX_TASKS) {
        return &task_stats[token];
    } else {
        return nullptr;
    }
}

static bool isTaskEnabled(const task_stats_t* stats)
{
    return stats->enabled;
}

static bool isTaskValid(const task_stats_t* stats)
{
    return stats->name != nullptr && stats->interval_sec > 0;
}

static teller::supervisor::task_token_t registerTask(const char* name)
{
    task_stats_t* stats;
    lock_guard lock(stats_mutex);

    if (!name) {
        return INVALID_TOKEN;
    }

    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        stats = &task_stats[i];
        if (!isTaskValid(stats)) {
            stats->name = name;
            stats->interval_sec = 1;
            stats->time_until_next_check_sec = 1;
            stats->min_nudges = 0;
            stats->max_nudges = std::numeric_limits<uint16_t>::max();
            return static_cast<teller::supervisor::task_token_t>(i);
        }
    }

    return INVALID_TOKEN;
}

static void unregisterTask(teller::supervisor::task_token_t token)
{
    lock_guard lock(stats_mutex);

    task_stats_t* stats = getTaskFromToken(token);
    if (stats) {
        memset(stats, 0, sizeof(task_stats_t));
    }
}

static void disableTask(teller::supervisor::task_token_t token)
{
    task_stats_t* stats = getTaskFromToken(token);
    if (stats) {
        stats->nudges = 0;
        stats->enabled = false;
    }
}

static void nudgeTask(teller::supervisor::task_token_t token)
{
    task_stats_t* stats = getTaskFromToken(token);
    if (stats) {
        stats->nudges++;
        stats->enabled = true;
    }
}

/* ************************************************************************** */

static queue_stats_t* getQueueFromToken(teller::supervisor::queue_token_t token)
{
    if (token >= 0 && token < MAX_QUEUES) {
        return &queue_stats[token];
    } else {
        return nullptr;
    }
}

static bool isQueueValid(const queue_stats_t* stat)
{
    return stat->name != nullptr;
}

static teller::supervisor::queue_token_t registerQueue(const char* name, BlockingQueueBase* queue)
{
    lock_guard lock(stats_mutex);
    queue_stats_t* stats;

    if (!name || !queue) {
        return INVALID_TOKEN;
    }

    for (uint8_t i = 0; i < MAX_QUEUES; i++) {
        stats = &queue_stats[i];
        if (!isQueueValid(stats)) {
            stats->name = name;
            stats->queue = queue;
        }
    }

    return INVALID_TOKEN;
}

static void unregisterQueue(teller::supervisor::queue_token_t token)
{
    lock_guard lock(stats_mutex);
    queue_stats_t* stats = getQueueFromToken(token);
    if (stats) {
        memset(stats, 0, sizeof(queue_stats_t));
    }
}

/* ************************************************************************** */
