#include <gtest/gtest.h>

#include "hal/gpio.h"

using namespace teller::hal::gpio;

class GPIOTest : public testing::Test {
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

TEST_F(GPIOTest, readAndWrite)
{
    EXPECT_FALSE(read(LO));
    write(LO, true);
    EXPECT_TRUE(read(LO));
    write(LO, false);
    EXPECT_FALSE(read(LO));
}

TEST_F(GPIOTest, initialLCLStatus)
{
    EXPECT_TRUE(read(STATUS_GMM_LCL));
    EXPECT_TRUE(read(STATUS_SCM_LCL));
    EXPECT_TRUE(read(STATUS_SUC_LCL1));
    EXPECT_TRUE(read(STATUS_SUC_LCL2));
    EXPECT_TRUE(read(STATUS_SUC_LCL3));
    EXPECT_TRUE(read(STATUS_HVPSU_LCL));
}
