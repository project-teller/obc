#include <cstring>
#include <gtest/gtest.h>

#include "core/rxsm.h"

using namespace teller::rxsm;
using namespace teller::rxsm::signal;

TEST(StateManager, initialization)
{
    StateManager manager;
    State state;

    state.lo = true;
    state.sods = true;
    state.soe = true;

    manager.getState(state);
    EXPECT_FALSE(state.lo);
    EXPECT_FALSE(state.soe);
    EXPECT_FALSE(state.sods);
}

TEST(StateManager, update)
{
    /* clang-format off */
    uint8_t updatesAndExpected[] = {
        /* Values come in pairs */
        /* First column is the update, second is the majority decision */
        0,               0,
        SOE,             0,
        SOE,             0,
        SOE,             SOE,
        SOE | SODS,      SOE,
        SOE | SODS,      SOE,
        SOE,             SOE,
        SOE | SODS,      SOE | SODS,
        SOE | SODS | LO, SOE | SODS,
        SOE | SODS,      SOE | SODS,
        SOE | SODS | LO, SOE | SODS,
        SOE | SODS,      SOE | SODS,
        SOE | SODS | LO, SOE | SODS | LO,
        SOE | SODS | LO, SOE | SODS | LO,
        SODS | LO,       SOE | SODS | LO,
        SODS | LO,       SOE | SODS | LO,
        SOE | SODS | LO, SOE | SODS | LO,
        SODS | LO,       SODS | LO,
    };
    /* clang-format on */

    StateManager manager;
    State state;
    size_t n = (sizeof(updatesAndExpected) / sizeof(updatesAndExpected[0])) >> 1;
    size_t i;

    for (i = 0; i < n; i++) {
        manager.update(updatesAndExpected[i * 2]);
        manager.getState(state);

        EXPECT_EQ(updatesAndExpected[i * 2 + 1] & LO, state.lo ? LO : 0);
        EXPECT_EQ(updatesAndExpected[i * 2 + 1] & SODS, state.sods ? SODS : 0);
        EXPECT_EQ(updatesAndExpected[i * 2 + 1] & SOE, state.soe ? SOE : 0);
    }
}
