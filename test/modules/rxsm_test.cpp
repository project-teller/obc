#include <cstring>
#include <gtest/gtest.h>

#include "modules/rxsm.h"

using namespace teller::rxsm;
using namespace teller::rxsm::signal;

class RXSMTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(init());
    }

    void TearDown() override
    {
        destroy();
    }
};

TEST_F(RXSMTest, initialization)
{
    State state;

    state.lo = true;
    state.sods = true;
    state.soe = true;

    getState(state);
    EXPECT_FALSE(state.lo);
    EXPECT_FALSE(state.soe);
    EXPECT_FALSE(state.sods);
}

TEST_F(RXSMTest, update)
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

    State state;
    size_t n = (sizeof(updatesAndExpected) / sizeof(updatesAndExpected[0])) >> 1;
    size_t i;

    for (i = 0; i < n; i++) {
        update(
            updatesAndExpected[i * 2] & SODS,
            updatesAndExpected[i * 2] & SOE,
            updatesAndExpected[i * 2] & LO);
        getState(state);

        EXPECT_EQ(updatesAndExpected[i * 2 + 1] & LO, state.lo ? LO : 0);
        EXPECT_EQ(updatesAndExpected[i * 2 + 1] & SODS, state.sods ? SODS : 0);
        EXPECT_EQ(updatesAndExpected[i * 2 + 1] & SOE, state.soe ? SOE : 0);
    }
}
