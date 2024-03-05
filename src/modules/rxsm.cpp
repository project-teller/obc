#include "modules/rxsm.h"

using namespace teller::rxsm;

namespace teller::rxsm {

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
     * @brief Resets the state of the state manager.
     */
    void reset(void);

    /**
     * @brief Updates the state of all signals at once.
     */
    void update(bool sods_, bool soe_, bool lo_);

private:
    /** Majority voter for the SODS signal */
    teller::utils::MajorityVoter sods;

    /** Majority voter for the SOE signal */
    teller::utils::MajorityVoter soe;

    /** Majority voter for the LO signal */
    teller::utils::MajorityVoter lo;
};

void StateManager::getState(State& state) const
{
    state.lo = lo.get();
    state.sods = sods.get();
    state.soe = soe.get();
}

void StateManager::reset()
{
    lo.reset();
    sods.reset();
    soe.reset();
}

void StateManager::update(bool sods_, bool soe_, bool lo_)
{
    lo.feed(lo_);
    sods.feed(sods_);
    soe.feed(soe_);
}

static StateManager rxsmStateManager;

bool init()
{
    rxsmStateManager.reset();
    return true;
}

void destroy()
{
    rxsmStateManager.reset();
}

void getState(State& state)
{
    rxsmStateManager.getState(state);
}

void update(bool sods, bool soe, bool lo)
{
    rxsmStateManager.update(sods, soe, lo);
}

}
