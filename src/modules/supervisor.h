#pragma once

#include <cstdint>
#include <limits>

#include "hal/queue.hpp"

namespace teller::supervisor {

typedef uint8_t task_token_t;
typedef uint8_t queue_token_t;

/**
 * @brief Class that represents the registration of a task in the supervisor.
 */
class TaskRegistration {

public:
    /**
     * @brief Registers a task in the supervisor.
     *
     * @param name  the name of the task
     * @param interval_sec  number of seconds between consecutive checks of the task
     * @param min_nudges  minimum number of times a task must nudge the supervisor
     *        between two checks to consider it healthy
     * @param max_nudges  maximum number of times a task must nudge the supervisor
     *        between two checks to consider it healthy; 0xFFFF for unlimited
     */
    TaskRegistration(const char* name);
    ~TaskRegistration();

    /**
     * @brief Disables the task registration.
     *
     * No errors are reported from a disabled task registration. A nudge enables
     * the task registration automatically.
     */
    void disable(void);

    /**
     * @brief Sets the number of nudges that are to be expected between consecutive checks.
     */
    TaskRegistration& expect(std::uint16_t min, std::uint16_t max = std::numeric_limits<uint16_t>::max());

    /**
     * @brief Sets the time interval that should pass between consecutive checks
     * to the given value.
     */
    TaskRegistration& inSeconds(std::uint8_t interval);

    /**
     * @brief Nudges the supervisor to notify it that a given task has completed an iteration.
     */
    void nudge(void);

private:
    task_token_t m_token;
};

/**
 * @brief Class that represents the registration of a queue in the supervisor.
 */
class QueueRegistration {

public:
    /**
     * @brief Registers a queue in the supervisor.
     *
     * @param name  the name of the queue
     * @param queue the queue to register
     */
    QueueRegistration(const char* name, teller::hal::BlockingQueueBase* queue);
    ~QueueRegistration();

private:
    queue_token_t m_token;
};

/**
 * @brief Initializes the task and watchdog supervisor of the REXUS service module.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the task and watchdog supervisor of the REXUS service module.
 */
void destroy(void);

/**
 * @brief Configures the watchdog before entering the main loop of the supervisor task.
 */
void setup(void);

/**
 * @brief Checks the status of each registered queue to detect overflows.
 * @return true if none of the queues are overflowing, false otherwise
 */
bool checkQueues(void);

/**
 * @brief Checks the status of each registered task to detect deadlocks.
 *
 * @param  timestamp  the current timestamp; zero if it should be queried from the HAL
 * @return true if all the tasks are running at their expected frequencies, false otherwise
 */
bool checkTasks(uint32_t timestamp = 0);

};
