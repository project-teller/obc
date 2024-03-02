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
    EXPECT_FALSE(read(GPIO_LO));
    write(GPIO_LO, true);
    EXPECT_TRUE(read(GPIO_LO));
    write(GPIO_LO, false);
    EXPECT_FALSE(read(GPIO_LO));
}
