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
