#include "modules/rxsm.h"

#include "core/log_records.h"
#include "core/telem/generic.h"
#include "hal/system.h"
#include "modules/edr.hpp"
#include "modules/scheduler.h"

using namespace teller::log;
using namespace teller::rxsm;
using teller::telem::storage_area_t;

static teller::edr::FormattedLogRecord<uint32_t, bool, bool, bool> logRecord(
    LOG_RECORD_RXSM, "RXSM", "TimeMS,SODS,SOE,LO", "IBBB", "s---", "C---");

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
     *
     * @return Bitfield indicating the signals for which the state has changed,
     *         taking into account the majority votes.
     */
    signal::signal_t update(bool sods_, bool soe_, bool lo_);

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

signal::signal_t StateManager::update(bool sods_, bool soe_, bool lo_)
{
    uint8_t changed = 0;

    if (lo.feedAndCheck(lo_)) {
        changed |= signal::LO;
    }

    if (sods.feedAndCheck(sods_)) {
        changed |= signal::SODS;
    }

    if (soe.feedAndCheck(soe_)) {
        changed |= signal::SOE;
    }

    return static_cast<signal::signal_t>(changed);
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
    signal::signal_t changed = rxsmStateManager.update(sods, soe, lo);

    if (changed) {
        logCurrentState();
    }

    if (changed & signal::SOE) {
        /* SOE signal triggers the scheduler */
        State state;
        rxsmStateManager.getState(state);

        if (state.soe) {
            teller::scheduler::reset();
            teller::scheduler::start();
        } else {
            teller::scheduler::stop();
        }
    }
}

static void logCurrentState()
{
    State state;

    rxsmStateManager.getState(state);
    logRecord.write(
        teller::hal::system::getTimeSinceBootMsec(),
        state.sods, state.soe, state.lo);
}

static void onLogOpened(storage_area_t area)
{
    logCurrentState();
}

}
