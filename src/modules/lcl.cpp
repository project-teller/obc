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
     * Indicates that we have given up on this LCL, at least for the current
     * liftoff.
     */
    AUTO_RESET_GIVEN_UP = 4
} lcl_auto_reset_state_t;

/** Auxiliary information for tracking the auto-reset logic of the LCLs */
typedef struct {
    lcl_auto_reset_state_t state;
    uint8_t reset_counter;
} lcl_auto_reset_t;

static Logger* logger;

/** Stores when the last liftoff has started */
static uint32_t last_liftoff_started_at = 0;

/** Stores the state of the auto-reset logic of the LCLs */
static lcl_auto_reset_t autoReset[teller::lcl::NUM_LCLS];

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

void reset(lcl_t lcl)
{
    if (lcl >= 0 && lcl < NUM_LCLS) {
        logger->info("Resetting %s LCL", lcl_names[lcl]);
    }
    resetMultiple(1 << static_cast<uint8_t>(lcl));
}

void resetMultiple(uint8_t lcls_to_reset)
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
    last_liftoff_started_at = 0;
    for (int i = 0; i < teller::lcl::NUM_LCLS; i++) {
        autoReset[i].state = AUTO_RESET_DISABLED;
        autoReset[i].reset_counter = 0;
    }
}

static void updateAutoResetLogicAfterTakeoff()
{
    /* TODO(ntamas) */
}

static void updateAutoResetLogicBeforeTakeoff()
{
    /* TODO(ntamas) */
}
