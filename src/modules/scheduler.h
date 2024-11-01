#pragma once

#include <cstdint>

namespace teller::scheduler {

/**
 * @brief Initializes the event scheduler of the OBC.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the event scheduler of the OBC.
 */
void destroy(void);

/**
 * @brief Returns the time elapsed on the scheduler clock, in milliseconds.
 *
 * @param  now  the current timestamp; zero means to query it from the system
 */
uint32_t getElapsedTimeMsec(uint32_t now = 0);

/**
 * @brief Returns whether the scheduler clock is currently running.
 */
bool isRunning(void);

/**
 * @brief Resets the scheduler clock to zero seconds.
 */
void reset(void);

/**
 * @brief Starts the scheduler clock.
 */
void start(void);

/**
 * @brief Stops the scheduler clock.
 */
void stop(void);

/**
 * @brief Updates the scheduler and executes pending events.
 */
void update(void);

};
