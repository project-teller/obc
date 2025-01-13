#include <limits>

#include "core/telem/calibration.h"
#include "hal/gpio.h"
#include "hal/system.h"
#include "modules/cam.h"
#include "modules/cmd.h"
#include "modules/lcl.h"
#include "modules/scheduler.h"
#include "modules/telem.h"

using namespace teller::rxsm::signal;

namespace teller::scheduler {

/**
 * @brief Possible types of events in the scheduler.
 */
typedef enum {
    EVENT_NOP = 0,
    EVENT_END,
    EVENT_LCL_RESET,
    EVENT_GPIO_SET,
    EVENT_GPIO_CLEAR,
    EVENT_CALIBRATION,
    EVENT_ENABLE_DISABLE_CAMERA,
    EVENT_TOGGLE_CAMERA,
    EVENT_SET_TELEMETRY_LEVEL,
} scheduler_event_type_t;

/**
 * @brief Flags that can be attached to an event.
 */
typedef enum {
    /**
     * Flag indicating that the event should be ignored when we detect that
     * the OBC was (re)booted after liftoff.
     *
     * This flag should be used only on events that are scheduled at least 3
     * seconds later than the SODS signal. This is because we typically need
     * a few seconds after boot to detect whether we were booted after liftoff
     * or not.
     */
    EVENT_FLAG_IGNORE_WHEN_BOOTED_AFTER_LIFTOFF = 1
} scheduler_event_flag_t;

/**
 * @brief Structure representing a single event in the scheduler.
 */
typedef struct scheduler_event_s {
    /** The timestamp of the event, in milliseconds since the SODS signal */
    uint32_t timestamp_msec;

    /** The type of the event */
    scheduler_event_type_t type;

    /** The paramter of the event (if any) The exact meaning depends on the event type. */
    uint32_t param;

    /** Additional flags of the event */
    uint8_t flags;
} scheduler_event_t;

#define FUTURE std::numeric_limits<uint32_t>::max()
#define NO_MORE_EVENTS    \
    {                     \
        FUTURE, EVENT_END \
    }

/**
 * @brief Array of events that the scheduler will execute relative to SOE.
 *
 * The array must be sorted by timestamp.
 */
static const scheduler_event_t eventsRelativeToSOE[] = {
    /* SOE signal: turn on GMM and SUC, ensure full telemetry */
    { 0, EVENT_LCL_RESET, teller::lcl::GMM_LCL },
    { 0, EVENT_LCL_RESET, teller::lcl::SUC_LCL1 },
    { 0, EVENT_LCL_RESET, teller::lcl::SUC_LCL2 },
    { 0, EVENT_LCL_RESET, teller::lcl::SUC_LCL3 },
    { 0, EVENT_SET_TELEMETRY_LEVEL, teller::telem::TELEMETRY_LEVEL_FULL },

    /* SOE+1s: turn on SCM */
    { 1000, EVENT_LCL_RESET, teller::lcl::SCM_LCL },

    /* SOE+15s, T-255: calibrate gyroscope */
    { 15000, EVENT_CALIBRATION, teller::telem::frames::CALIBRATION_GYRO,
        EVENT_FLAG_IGNORE_WHEN_BOOTED_AFTER_LIFTOFF },

    /* SOE+30s, T-240: enable camera LCL */
    { 30000, EVENT_LCL_RESET, teller::lcl::CAM_LCL },

    NO_MORE_EVENTS,
};

/**
 * @brief Array of events that the scheduler will execute relative to SODS.
 *
 * The array must be sorted by timestamp.
 */
static const scheduler_event_t eventsRelativeToSODS[] = {
    { 0, EVENT_SET_TELEMETRY_LEVEL, teller::telem::TELEMETRY_LEVEL_FULL },
    NO_MORE_EVENTS,
};

/**
 * @brief Array of events that the scheduler will execute relative to liftoff.
 *
 * The array must be sorted by timestamp.
 */
static const scheduler_event_t eventsRelativeToLiftoff[] = {
    { 0, EVENT_SET_TELEMETRY_LEVEL, teller::telem::TELEMETRY_LEVEL_FULL },
    NO_MORE_EVENTS,
};

static struct {
    teller::scheduler::Scheduler sods;
    teller::scheduler::Scheduler soe;
    teller::scheduler::Scheduler lo;
} schedulerList;

static void executeEvent(const scheduler_event_t& event);

bool init(void)
{
    schedulerList.sods.setEvents(eventsRelativeToSODS);
    schedulerList.soe.setEvents(eventsRelativeToSOE);
    schedulerList.lo.setEvents(eventsRelativeToLiftoff);
    return true;
}

void destroy(void)
{
    schedulerList.sods.clearEvents();
    schedulerList.soe.clearEvents();
    schedulerList.lo.clearEvents();
}

Scheduler* getMissionClock()
{
    return &schedulerList.soe;
}

Scheduler* getSchedulerForRXSMSignal(signal_t signal)
{
    switch (signal) {
    case SOE:
        return &schedulerList.soe;
    case SODS:
        return &schedulerList.sods;
    case LO:
        return &schedulerList.lo;
    }

    return nullptr;
}

void update(void)
{
    schedulerList.soe.executePendingEvents();
    schedulerList.sods.executePendingEvents();
    schedulerList.lo.executePendingEvents();
}

Scheduler::Scheduler()
    : startedAt(0)
    , stoppedAt(0)
    , isClockRunning(false)
    , events(nullptr)
    , nextEventIndex(0)
{
    stop();
    reset();
}

Scheduler::~Scheduler()
{
    clearEvents();
}

void Scheduler::clearEvents()
{
    setEvents(nullptr);
}

uint32_t Scheduler::getElapsedTimeMsec(uint32_t now) const
{
    if (isClockRunning) {
        now = now ? now : this->now();
        return now - startedAt;
    } else {
        return stoppedAt - startedAt;
    }
}

/**
 * @brief Sets the table of events for this scheduler.
 *
 * Should be called only once when the scheduler is initialized.
 */
void Scheduler::setEvents(const scheduler_event_t* events)
{
    if (events == this->events) {
        return;
    }

    stop();
    reset();

    this->events = events;
}

void Scheduler::reset()
{
    startedAt = isRunning() ? now() : 0;
    stoppedAt = 0;
    nextEventIndex = 0;
}

void Scheduler::start()
{
    if (!isClockRunning) {
        startedAt = now() - getElapsedTimeMsec();
        stoppedAt = 0;
        isClockRunning = true;
    }
}

void Scheduler::stop()
{
    if (isClockRunning) {
        stoppedAt = now();
        isClockRunning = false;
    }
}

uint32_t Scheduler::now() const
{
    return teller::hal::system::getTimeSinceBootMsec();
}

void Scheduler::executePendingEvents()
{
    uint32_t elapsedTime;
    const scheduler_event_t* nextEvent;

    if (!isClockRunning || !events) {
        return;
    }

    elapsedTime = getElapsedTimeMsec();

    while (true) {
        nextEvent = &events[nextEventIndex];
        if (nextEvent->type == EVENT_END || nextEvent->timestamp_msec > elapsedTime) {
            break;
        }

        executeEvent(*nextEvent);
        nextEventIndex++;
    }
}

static void executeEvent(const scheduler_event_t& event)
{
    if (event.flags & EVENT_FLAG_IGNORE_WHEN_BOOTED_AFTER_LIFTOFF) {
        if (teller::rxsm::wasBootedAfterLiftoff()) {
            /* Ignore this event */
            return;
        }
    }

    switch (event.type) {
    case EVENT_LCL_RESET:
        /* Resetting LCL corresponding to the parameter */
        if (event.param < teller::lcl::NUM_LCLS) {
            teller::lcl::reset(static_cast<teller::lcl::lcl_t>(event.param));
        }
        break;

    case EVENT_GPIO_SET:
    case EVENT_GPIO_CLEAR:
        /* Set GPIO pin to high or low */
        if (event.param < teller::hal::gpio::NUM_GPIO_PINS) {
            teller::hal::gpio::write(
                static_cast<teller::hal::gpio::pin_t>(event.param),
                event.type == EVENT_GPIO_SET);
        }
        break;

    case EVENT_CALIBRATION:
        /* Calibrate a sensor */
        if (event.param < teller::telem::frames::NUM_CALIBRATION_PROCEDURES) {
            teller::cmd::performCalibration(
                static_cast<teller::telem::frames::calibration_procedure_t>(event.param));
        }
        break;

    case EVENT_ENABLE_DISABLE_CAMERA:
        /* Enable or disable the camera */
        teller::cam::setEnabled(event.param != 0);
        break;

    case EVENT_TOGGLE_CAMERA:
        /* Send a pulse to toggle the camera unconditionally */
        teller::cam::sendPulse();
        break;

    case EVENT_SET_TELEMETRY_LEVEL:
        /* Set the telemetry level */
        teller::telem::setTelemetryLevel(
            static_cast<teller::telem::telemetry_level_t>(event.param));
        break;

    default:
        break;
    }
}

}
