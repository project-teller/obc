#include <ctime>
#include <gtest/gtest.h>

#include "hal/system.h"

using namespace teller::hal::system;

int millisleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

class SystemTest : public testing::Test {
protected:
    void SetUp() override
    {
        init();
    }

    void TearDown() override
    {
        destroy();
    }
};

TEST_F(SystemTest, getTimeSinceBootMsec)
{
    uint32_t timestamps[4];
    uint8_t i;

    for (i = 0; i < 4; i++) {
        if (i != 0) {
            millisleep(2);
        }
        timestamps[i] = getTimeSinceBootMsec();
        printf("%ld\n", (long int)timestamps[i]);
    }

    EXPECT_TRUE(timestamps[1] > timestamps[0]);
    EXPECT_TRUE(timestamps[2] > timestamps[1]);
    EXPECT_TRUE(timestamps[3] > timestamps[2]);

    EXPECT_TRUE(timestamps[3] - timestamps[0] < 10);
}

TEST_F(SystemTest, getReasonOfLastReset)
{
    EXPECT_EQ(RESET_REASON_NORMAL, getReasonOfLastReset());
}
