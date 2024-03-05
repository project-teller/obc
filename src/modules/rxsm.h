#pragma once

#include <cstdint>

#include "core/utils/majority_voter.h"

namespace teller::rxsm {

namespace signal {

    /**
     * @brief Symbolic names for the REXUS service module signals.
     *
     * These constants are meant to be used as bits in a bitfield.
     */
    typedef enum {
        SODS = 1,
        SOE = 2,
        LO = 4
    } signal_t;

}

/**
 * @brief Class representing the states of the signals from the REXUS service module.
 */
struct State {
    bool sods;
    bool soe;
    bool lo;
};

/**
 * @brief Initializes the state manager of the REXUS service module.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the state manager of the REXUS service module.
 */
void destroy(void);

/**
 * @brief Returns the current state of all signals into the given state object.
 */
void getState(State& state);

/**
 * @brief Updates the state of all signals at once.
 *
 * @param sods  the current value of the SODS signal
 * @param soe   the current value of the SOE signal
 * @param lo    the current value of the LO signal
 */
void update(bool sods, bool soe, bool lo);

};
