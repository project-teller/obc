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
 * @brief State updater mechanism for the signals of the REXUS service module.
 *
 * The state updater implements a majority-of-five rule for each of the signals.
 */
class StateManager {
public:
    StateManager() { }

    /**
     * @brief Returns the current state of all signals into the given state object.
     */
    void getState(State& state) const;

    /**
     * @brief Updates the state of all signals at once.
     *
     * @param signals  a value where bit 0 corresponds to the SODS, bit 1
     *        corresponds to the SOE and bit 2 corresponds to the LO signal.
     *        Use the symbolic constants from \ref teller::rxsm::signal::signal_t
     *        for sake of readability.
     */
    void update(uint8_t signals);

private:
    /** Majority voter for the SODS signal */
    teller::utils::MajorityVoter sods;

    /** Majority voter for the SOE signal */
    teller::utils::MajorityVoter soe;

    /** Majority voter for the LO signal */
    teller::utils::MajorityVoter lo;
};

};
