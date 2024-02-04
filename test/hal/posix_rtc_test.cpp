#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/time.h"
#include "hal/rtc.h"

using namespace teller::hal::rtc;

class RTCTest : public testing::Test {
protected:
    void SetUp() override
    {
        EXPECT_TRUE(init());
    }

    void TearDown() override
    {
        destroy();
    }
};

TEST_F(RTCTest, get)
{
    uint64_t expected, observed;
    struct timespec now;

    clock_gettime(CLOCK_REALTIME, &now);
    EXPECT_LT(getTimeMsec() - timespecToMsec(&now), 500);
}

TEST_F(RTCTest, set)
{
    EXPECT_FALSE(setTimeMsec(1234));
}
