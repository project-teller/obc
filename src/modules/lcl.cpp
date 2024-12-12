#include "modules/lcl.h"
#include "modules/log.h"

#include "hal/gpio.h"
#include "hal/system.h"

using namespace teller::hal::gpio;
using namespace teller::log;
using namespace teller::telem;

/**
 * @brief Pair of GPIO pins associated to an LCL.
 *
 * Each LCL has two pins: a query pin and a reset pin.
 */
typedef struct {
    pin_t query_pin;
    pin_t reset_pin;
} lcl_pins_t;

/** Pulse duration to use for resetting an LCL. Used to speed up unit tests. */
static uint16_t pulse_duration_msec;

/** Idle state of the LLCs */
static const bool IDLE = false;

/** Active state of the LLCs */
static const bool ACTIVE = (!IDLE);

/** Delay between consecutive auto-resets of a LCL, in milliseconds */
static const int DELAY_BETWEEN_AUTO_RESETS_MSEC = 3000;

/** Delay between trigger and auto-reset of a LCL, in milliseconds */
static const int DELAY_BEFORE_AUTO_RESET_MSEC = 3000;

/** Maximum number of resets allowed */
static const int MAX_RESET_COUNT = 10;

static const int NUM_LCLS = teller::lcl::NUM_LCLS;

typedef enum {
    /**
     * Indicates that the auto-reset is disabled for this LCL because
     * the liftoff signal has not happened yet.
     */
    AUTO_RESET_DISABLED = 0,

    /**
     * Indicates that the auto-reset is disabled for this LCL because
     * it was not enabled when the liftoff signal triggered.
     */
    AUTO_RESET_OFF = 1,

    /**
     * Indicates that the auto-reset is enabled for this LCL and the LCL has
     * not triggered yet (so there is no need to reset).
     */
    AUTO_RESET_ON = 2,

    /**
     * Indicates that the auto-reset is enabled for this LCL and it was
     * triggered recently so we need to wait until we try it again.
     */
    AUTO_RESET_TRIED_RECENTLY = 3,

    /**
     * Indicates that an LCL was triggered but we have to wait a bit
     * before trying to turn it back on.
     */
    AUTO_RESET_WAITING = 4,

    /**
     * Indicates that we have given up on this LCL, at least for the current
     * liftoff.
     */
    AUTO_RESET_GIVEN_UP = 5
} lcl_auto_reset_state_t;

/** Auxiliary information for tracking the auto-reset logic of the LCLs */
typedef struct {
    lcl_auto_reset_state_t state;
    uint32_t lastTriggerAt;
    uint32_t lastResetAt;
    uint8_t resetCounter;
} lcl_auto_reset_t;

static Logger* logger;

/** Stores when the last liftoff has started */
static uint32_t lastLiftoffStartedAtMsec = 0;

/** Stores the state of the auto-reset logic of the LCLs */
static lcl_auto_reset_t autoReset[NUM_LCLS];

static void clearAutoResetLogic(void);
static void updateAutoResetLogicAfterTakeoff(void);
static void updateAutoResetLogicBeforeTakeoff(void);

namespace teller::lcl {

/**
 * @brief Table mapping LCL indices to their GPIO pins.
 */
static const lcl_pins_t pin_map[NUM_LCLS] = {
    { STATUS_GMM_LCL, RST_GMM_LCL },
    { STATUS_SCM_LCL, RST_SCM_LCL },
    { STATUS_SUC_LCL1, RST_SUC_LCL1 },
    { STATUS_SUC_LCL2, RST_SUC_LCL2 },
    { STATUS_SUC_LCL3, RST_SUC_LCL3 },
    { STATUS_CAM_LCL, RST_CAM_LCL },
};

/**
 * @brief Names of the LCLs.
 */
static const char* lcl_names[NUM_LCLS] = {
    "GMM", "SCM", "SUC1", "SUC2", "SUC3", "CAM"
};

bool init()
{
    pulse_duration_msec = 100;
    clearAutoResetLogic();

    /* LCL reset pins start from the idle state */
    for (int i = 0; i < NUM_LCLS; i++) {
        pin_t pin = pin_map[static_cast<lcl_t>(i)].reset_pin;
        write(pin, IDLE);
    }

    logger = getLogger(MODULE_ID_OBC);
    return logger != nullptr;

    return true;
}

void destroy()
{
    clearAutoResetLogic();
}

bool triggered(lcl_t lcl)
{
    if (lcl >= 0 && lcl < NUM_LCLS) {
        return read(pin_map[lcl].query_pin);
    } else {
        return false;
    }
}

void reset(lcl_t lcl, lcl_reset_reason_t reason)
{
    if (lcl >= 0 && lcl < NUM_LCLS) {
        if (reason == RESET_REASON_AUTO) {
            logger->info("Auto-resetting %s LCL", lcl_names[lcl]);
        } else {
            logger->info("Resetting %s LCL", lcl_names[lcl]);
        }
    }
    resetMultiple(1 << static_cast<uint8_t>(lcl));
}

void resetMultiple(uint8_t lcls_to_reset, lcl_reset_reason_t reason)
{
    bool hasAtLeastOne = false;

    for (uint8_t lcl = 0; lcl < NUM_LCLS; lcl++) {
        if (lcls_to_reset & (1 << lcl)) {
            pin_t pin = pin_map[lcl].reset_pin;
            write(pin, ACTIVE);
            hasAtLeastOne = true;
        }
    }

    if (hasAtLeastOne) {
        teller::hal::system::delayMsec(pulse_duration_msec);
    }

    for (uint8_t lcl = 0; lcl < NUM_LCLS; lcl++) {
        if (lcls_to_reset & (1 << lcl)) {
            pin_t pin = pin_map[lcl].reset_pin;
            write(pin, IDLE);
        }
    }
}

void setResetPulseDurationMsec(uint16_t duration_msec)
{
    pulse_duration_msec = duration_msec;
}

void updateAutoResetLogic(const teller::rxsm::State& state)
{
    if (state.lo) {
        updateAutoResetLogicAfterTakeoff();
    } else {
        updateAutoResetLogicBeforeTakeoff();
    }
}

}

static void clearAutoResetLogic(void)
{
    lastLiftoffStartedAtMsec = 0;
    for (int i = 0; i < NUM_LCLS; i++) {
        autoReset[i].state = AUTO_RESET_DISABLED;
        autoReset[i].resetCounter = 0;
        autoReset[i].lastResetAt = 0;
        autoReset[i].lastTriggerAt = 0;
    }
}

static void updateAutoResetLogicAfterTakeoff()
{
    int i;
    bool triggered;

    /* When LO has just turned on now, we need to record which LCLs are
     * currently in their nominal state and we only need to track those */
    if (lastLiftoffStartedAtMsec == 0) {
        lastLiftoffStartedAtMsec = teller::hal::system::getTimeSinceBootMsec();
        for (i = 0; i < NUM_LCLS; i++) {
            triggered = teller::lcl::triggered(static_cast<teller::lcl::lcl_t>(i));
            if (triggered) {
                autoReset[i].state = AUTO_RESET_OFF;
            } else {
                autoReset[i].state = AUTO_RESET_ON;
            }
            autoReset[i].resetCounter = 0;
            autoReset[i].lastResetAt = 0;
            autoReset[i].lastTriggerAt = 0;
        }
    } else {
        uint32_t now = teller::hal::system::getTimeSinceBootMsec();
        for (i = 0; i < NUM_LCLS; i++) {
            triggered = teller::lcl::triggered(static_cast<teller::lcl::lcl_t>(i));
            switch (autoReset[i].state) {
            case AUTO_RESET_ON:
                if (triggered) {
                    if (autoReset[i].resetCounter >= MAX_RESET_COUNT) {
                        /* No more resets are allowed */
                        autoReset[i].state = AUTO_RESET_GIVEN_UP;
                        autoReset[i].resetCounter = 0;
                    } else {
                        /* Wait until timeout */
                        autoReset[i].lastTriggerAt = now;
                        autoReset[i].state = AUTO_RESET_WAITING;
                    }
                } else {
                    // autoReset[i].resetCounter = 0;
                }
                break;
            case AUTO_RESET_WAITING:
                if (!triggered) {
                    autoReset[i].state = AUTO_RESET_ON;
                } else if (now - autoReset[i].lastTriggerAt >= DELAY_BEFORE_AUTO_RESET_MSEC) {
                    /* Try a reset now */
                    teller::lcl::reset(
                        static_cast<teller::lcl::lcl_t>(i),
                        teller::lcl::RESET_REASON_AUTO);
                    autoReset[i].lastResetAt = now;
                    autoReset[i].state = AUTO_RESET_TRIED_RECENTLY;
                    autoReset[i].resetCounter++;
                }
                break;

            case AUTO_RESET_TRIED_RECENTLY:
                if (now - autoReset[i].lastResetAt >= DELAY_BETWEEN_AUTO_RESETS_MSEC) {
                    autoReset[i].state = AUTO_RESET_ON;
                }
                break;

            default:
                /* Nothing to do */
                break;
            }
        }
    }
}

static void updateAutoResetLogicBeforeTakeoff()
{
    /* When LO has just turned off now, we need to clear the state of the
     * auto-reset logic. Otherwise there is nothing to do */
    if (lastLiftoffStartedAtMsec != 0) {
        clearAutoResetLogic();
    }
}
