#include "modules/rxsm.h"
#include "core/telem/generic.h"
#include "modules/edr.hpp"

using namespace teller::rxsm;
using teller::telem::storage_area_t;

namespace teller::rxsm {

edr::FormattedLogRecord<bool, bool, bool> log(1, "RXSM", "sods,soe,lo", "BBB", "---");

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
     *
     * @return Whether the state of at least one signal changed conclusively,
     *         after taking into account the majority votes.
     */
    bool update(bool sods_, bool soe_, bool lo_);

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

bool StateManager::update(bool sods_, bool soe_, bool lo_)
{
    bool changed = false;

    changed |= lo.feedAndCheck(lo_);
    changed |= sods.feedAndCheck(sods_);
    changed |= soe.feedAndCheck(soe_);

    return changed;
}

static StateManager rxsmStateManager;

/**
 * @brief Posts a log message into the storage areas that contains the current
 * state of the REXUS service module signals.
 */
static void logCurrentState(void);

/**
 * @brief Handler that is called when a new log is opened.
 */
static void onLogOpened(storage_area_t area);

bool init()
{
    if (!edr::registerCallback(edr::EVENT_LOG_OPENED, &onLogOpened)) {
        return false;
    }

    rxsmStateManager.reset();
    return true;
}

void destroy()
{
    rxsmStateManager.reset();
    edr::unregisterCallback(edr::EVENT_LOG_OPENED, &onLogOpened);
}

void getState(State& state)
{
    rxsmStateManager.getState(state);
}

void update(bool sods, bool soe, bool lo)
{
    if (rxsmStateManager.update(sods, soe, lo)) {
        logCurrentState();
    }
}

static void logCurrentState()
{
    State state;

    rxsmStateManager.getState(state);
    log(state.sods, state.soe, state.lo);
}

static void onLogOpened(storage_area_t area)
{
    logCurrentState();
}

}
