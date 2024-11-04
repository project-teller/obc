#include <limits>

#include "core/telem/calibration.h"
#include "hal/gpio.h"
#include "hal/system.h"
#include "modules/cam.h"
#include "modules/cmd.h"
#include "modules/lcl.h"
#include "modules/scheduler.h"

static uint32_t startedAt = 0;
static uint32_t stoppedAt = 0;
static bool isClockRunning = false;

typedef enum {
    EVENT_NOP = 0,
    EVENT_END,
    EVENT_LCL_RESET,
    EVENT_GPIO_SET,
    EVENT_GPIO_CLEAR,
    EVENT_CALIBRATION,
    EVENT_ENABLE_DISABLE_CAMERA,
} scheduler_event_type_t;

typedef struct {
    uint32_t timestamp_msec;
    scheduler_event_type_t type;
    uint32_t param;
} scheduler_event_t;

#define FUTURE std::numeric_limits<uint32_t>::max()

/**
 * @brief Array of events that the scheduler will execute.
 *
 * The array must be sorted by timestamp.
 */
static const scheduler_event_t events[] = {
    /* SOE signal: turn on GMM and SCM */
    { 0, EVENT_LCL_RESET, teller::lcl::GMM_LCL },
    { 0, EVENT_LCL_RESET, teller::lcl::SCM_LCL },
    { 0, EVENT_LCL_RESET, teller::lcl::SUC_LCL1 },
    { 0, EVENT_LCL_RESET, teller::lcl::SUC_LCL2 },
    { 0, EVENT_LCL_RESET, teller::lcl::SUC_LCL3 },

    /* SOE+15s, T-255: calibrate gyroscope */
    { 15000, EVENT_CALIBRATION, teller::telem::frames::CALIBRATION_GYRO },

    /* SOE+30s, T-240: enable camera LCL */
    { 30000, EVENT_LCL_RESET, teller::lcl::CAM_LCL },

    /* SOE+40s, T-230: start camera recording */
    { 40000, EVENT_ENABLE_DISABLE_CAMERA, true },

    /* No more events */
    { FUTURE, EVENT_NOP },
};

#define NUM_EVENTS (sizeof(events) / sizeof(events[0]))

/**
 * @brief Playhead that points to the next event in the event array.
 */
static unsigned int nextEventIndex = 0;

static void executeEvent(const scheduler_event_t& event);

namespace teller::scheduler {

bool init(void)
{
    stop();
    reset();
    return true;
}

void destroy(void)
{
    stop();
    reset();
}

uint32_t getElapsedTimeMsec(uint32_t now)
{
    if (isClockRunning) {
        now = now ? now : teller::hal::system::getTimeSinceBootMsec();
        return now - startedAt;
    } else {
        return stoppedAt - startedAt;
    }
}

bool isRunning(void)
{
    return isClockRunning;
}

void reset(void)
{
    startedAt = isClockRunning ? teller::hal::system::getTimeSinceBootMsec() : 0;
    stoppedAt = 0;
    nextEventIndex = 0;
}

void start(void)
{
    if (!isClockRunning) {
        startedAt = teller::hal::system::getTimeSinceBootMsec() - getElapsedTimeMsec();
        stoppedAt = 0;
        isClockRunning = true;
    }
}

void stop(void)
{
    if (isClockRunning) {
        stoppedAt = teller::hal::system::getTimeSinceBootMsec();
        isClockRunning = false;
    }
}

void update(void)
{
    uint32_t elapsedTime;
    const scheduler_event_t* nextEvent;

    if (!isClockRunning) {
        /* No events are executed if the clock is not running */
        return;
    }

    elapsedTime = getElapsedTimeMsec();

    /* Execute pending events */
    while (nextEventIndex < NUM_EVENTS) {
        nextEvent = &events[nextEventIndex];

        if (nextEvent->timestamp_msec <= elapsedTime) {
            executeEvent(*nextEvent);
        } else {
            break;
        }

        nextEventIndex++;
    }
}

}

static void executeEvent(const scheduler_event_t& event)
{
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

    default:
        break;
    }
}
