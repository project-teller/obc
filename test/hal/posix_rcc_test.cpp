#include <ctime>
#include <gtest/gtest.h>

#include "hal/rcc.h"

using namespace teller::hal::rcc;

class RCCTest : public testing::Test {
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

TEST_F(RCCTest, getReasonOfLastReset)
{
    EXPECT_EQ(RESET_REASON_NORMAL, getReasonOfLastReset());
}
