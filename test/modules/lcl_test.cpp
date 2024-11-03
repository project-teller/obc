#include <cstring>
#include <gtest/gtest.h>

#include "hal/gpio.h"
#include "modules/lcl.h"
#include "modules/log.h"
#include "modules/telem.h"

using namespace teller;
using teller::lcl::lcl_t;

class LCLTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(hal::gpio::init());
        ASSERT_TRUE(telem::init());
        ASSERT_TRUE(log::init());
        ASSERT_TRUE(lcl::init());
        lcl::setResetPulseDurationMsec(1);
    }

    void TearDown() override
    {
        log::destroy();
        telem::destroy();
        lcl::destroy();
        hal::gpio::destroy();
    }
};

TEST_F(LCLTest, areTriggeredAfterBoot)
{
    for (int i = 0; i < lcl::NUM_LCLS; i++) {
        ASSERT_TRUE(lcl::triggered(static_cast<lcl_t>(i)));
    }

    /* Test invalid value */
    ASSERT_FALSE(lcl::triggered(lcl::NUM_LCLS));
}

TEST_F(LCLTest, resetLCL)
{
    for (int i = 0; i < lcl::NUM_LCLS; i++) {
        ASSERT_TRUE(lcl::triggered(static_cast<lcl_t>(i)));
        lcl::reset(static_cast<lcl_t>(i));
        ASSERT_FALSE(lcl::triggered(static_cast<lcl_t>(i)));
    }
}
