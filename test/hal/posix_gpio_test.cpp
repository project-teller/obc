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

TEST_F(GPIOTest, readAndWriteDigital)
{
    EXPECT_FALSE(readDigital(DGPIO_LO));
    writeDigital(DGPIO_LO, true);
    EXPECT_TRUE(readDigital(DGPIO_LO));
    writeDigital(DGPIO_LO, false);
    EXPECT_FALSE(readDigital(DGPIO_LO));
}

TEST_F(GPIOTest, readAndWriteAnalog)
{
    EXPECT_EQ(0, readAnalog(AGPIO_BOARD_VOLTAGE));
    writeAnalog(AGPIO_BOARD_VOLTAGE, 3300);
    EXPECT_EQ(3300, readAnalog(AGPIO_BOARD_VOLTAGE));
    writeAnalog(AGPIO_BOARD_VOLTAGE, 5000);
    EXPECT_EQ(5000, readAnalog(AGPIO_BOARD_VOLTAGE));
}

TEST_F(GPIOTest, readAndWriteAnalogInvalidIndex)
{
    EXPECT_EQ(0, readAnalog(AGPIO_COUNT));
    writeAnalog(AGPIO_COUNT, 3300);
    EXPECT_EQ(0, readAnalog(AGPIO_BOARD_VOLTAGE));
}
