#include "core/rxsm.h"

using namespace teller::rxsm;

namespace teller::rxsm {

void StateManager::getState(State& state) const
{
    state.lo = lo.get();
    state.sods = sods.get();
    state.soe = soe.get();
}

void StateManager::update(uint8_t signals)
{
    lo.feed(signals & signal::LO);
    sods.feed(signals & signal::SODS);
    soe.feed(signals & signal::SOE);
}

}