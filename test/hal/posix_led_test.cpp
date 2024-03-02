#include <gtest/gtest.h>

#include "hal/led.h"

using namespace teller::hal::led;

class LEDTest : public testing::Test {
protected:
    void SetUp() override
    {
        init();
    }

    void TearDown() override
    {
        destroy();
    }
};

TEST_F(LEDTest, setAndClear)
{
    EXPECT_FALSE(get(HEARTBEAT));
    EXPECT_FALSE(get(ERROR));

    set(ERROR, true);
    EXPECT_FALSE(get(HEARTBEAT));
    EXPECT_TRUE(get(ERROR));

    set(HEARTBEAT, true);
    EXPECT_TRUE(get(HEARTBEAT));
    EXPECT_TRUE(get(ERROR));

    clear(ERROR);
    EXPECT_TRUE(get(HEARTBEAT));
    EXPECT_FALSE(get(ERROR));
}
