#include <ctime>
#include <gtest/gtest.h>

#include "hal/hal.h"
#include "hal/led.h"

using namespace teller::hal;

class HALTest : public testing::Test {
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

TEST_F(HALTest, notifyFatalError)
{
    teller::hal::notifyFatalError();
    EXPECT_TRUE(led::get(led::ERROR));
}
