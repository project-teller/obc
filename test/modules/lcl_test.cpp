#include <cstring>
#include <gtest/gtest.h>

#include "hal/gpio.h"
#include "modules/lcl.h"

using namespace teller::lcl;

class LCLTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(teller::hal::gpio::init());
        ASSERT_TRUE(init());
        setResetPulseDurationMsec(1);
    }

    void TearDown() override
    {
        destroy();
        teller::hal::gpio::destroy();
    }
};

TEST_F(LCLTest, areTriggeredAfterBoot)
{
    for (int i = 0; i < NUM_LCLS; i++) {
        ASSERT_TRUE(triggered(static_cast<lcl_t>(i)));
    }

    /* Test invalid value */
    ASSERT_FALSE(triggered(NUM_LCLS));
}

TEST_F(LCLTest, resetLCL)
{
    for (int i = 0; i < NUM_LCLS; i++) {
        ASSERT_TRUE(triggered(static_cast<lcl_t>(i)));
        reset(static_cast<lcl_t>(i));
        ASSERT_FALSE(triggered(static_cast<lcl_t>(i)));
    }
}
