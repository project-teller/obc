#pragma once

#include <cstdint>

#include "core/telem/generic.h"

namespace teller::mode {

/**
 * @brief Enum listing the possible reasons of a mode change in the firmware.
 *
 * Multiple reasons can be ORed together.
 */
enum ModeChangeReasons : uint32_t {
    /** Any other reason not listed here */
    MODE_CHANGE_REASON_OTHER = 1,

    /** Mode change requested because the debug UART was connected or disconnected */
    MODE_CHANGE_REASON_DEBUG_UART = 2,

    /** Test mode requested from the ground station */
    MODE_CHANGE_REASON_GND_TEST_START = 4,

    /** Exiting from test mode requested from the ground station */
    MODE_CHANGE_REASON_GND_TEST_STOP = 8
};

const uint32_t MODE_CHANGE_REASON_ANY = 15;

/**
 * @brief Initializes the module handling whether the experiment is in mission or test mode.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the module handling the experiment mode.
 */
void destroy(void);

/**
 * @brief Returns the current experiment mode.
 */
teller::telem::obc_mode_t getMode(void);

/**
 * @brief Notifies the mode manager that the current mode should be re-evaluated.
 *
 * @param reason  reason explaining why the mode should be re-evaluated
 */
void notifyPossibleModeChange(ModeChangeReasons reason);

/**
 * @brief Updates the current experiment mode.
 *
 * This function must be called from some task at regular intervals. It blocks
 * until an event is triggered from some other module that would trigger a
 * re-evaluation of the current mode, and then updates the current mode
 * accordingly.
 */
void updateMode(void);

}
