#include <gtest/gtest.h>

#include "hal/watchdog.h"

using namespace teller::hal::watchdog;

class WatchdogTest : public testing::Test {
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

TEST_F(WatchdogTest, smokeTest)
{
    /* Smoke test */
    configureAndStart();
    reset();
}
