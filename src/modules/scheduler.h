#pragma once

#include <cstdint>

#include "modules/rxsm.h"

namespace teller::scheduler {

struct scheduler_event_s;

class Scheduler {

public:
    Scheduler();
    ~Scheduler();

    /**
     * @brief Clears the table of events for this scheduler.
     */
    void clearEvents();

    /**
     * @brief Returns the time elapsed since the start of the scheduler.
     *
     * @param now  the current timestamp
     */
    uint32_t getElapsedTimeMsec(uint32_t now = 0) const;

    /**
     * @brief Returns whether the clock of the scheduler is running.
     */
    bool isRunning() const
    {
        return this->isClockRunning;
    }

    /**
     * @brief Sets the table of events for this scheduler.
     *
     * Should be called only once when the scheduler is initialized.
     */
    void setEvents(const struct scheduler_event_s* events);

    /**
     * @brief Resets the clock of the scheduler.
     */
    void reset();

    /**
     * @brief Resets and starts the clock of the scheduler.
     */
    void restart()
    {
        reset();
        start();
    }

    /**
     * @brief Starts the clock of the scheduler.
     */
    void start();

    /**
     * @brief Stops the clock of the scheduler.
     */
    void stop();

    /**
     * @brief Execute pending events of the scheduler.
     *
     * This method calls the event executor on every event in the table of the
     * scheduler that have not been executed yet but whose timestamp is older
     * than the timestamp on the clock of the scheduler.
     */
    void executePendingEvents();

private:
    /**
     * Time when the scheduler was started, in milliseconds since boot.
     * The timestamps of events in the scheduler are calculated relative to
     * this event.
     */
    uint32_t startedAt;

    /**
     * Time when the scheduler was stopped, in milliseconds since boot.
     */
    uint32_t stoppedAt;

    /**
     * Stores whether the clock of the scheduler is running.
     */
    bool isClockRunning;

    /**
     * Events in the scheduler. Must end with a \c NO_MORE_EVENTS entry.
     */
    const struct scheduler_event_s* events;

    /**
     * Index of the next event to execute from the scheduler table.
     */
    unsigned int nextEventIndex;

    /**
     * @brief Returns the current time since boot.
     */
    uint32_t now() const;
};

/**
 * @brief Initializes the event scheduler of the OBC.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the event scheduler of the OBC.
 */
void destroy(void);

/**
 * @brief Returns the scheduler corresponding to the mission clock.
 *
 * Currently the mission clock is tied to the SOE signal.
 */
Scheduler* getMissionClock(void);

/**
 * @brief Returns the scheduler that is controlled by the given RXSM signal.
 *
 * Returns null if the given RXSM signal does not control any scheduler.
 */
Scheduler* getSchedulerForRXSMSignal(teller::rxsm::signal::signal_t signal);

/**
 * @brief Updates the scheduler and executes pending events.
 */
void update(void);

};
