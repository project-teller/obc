#include <cstring>
#include <gtest/gtest.h>

#include "hal/system.h"
#include "modules/messages.h"

using namespace teller::hal::system;
using namespace teller::telem;

class MessagesTest : public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(MessagesTest, updateTimesyncData)
{
    frames::timesync_data_t first;
    frames::timesync_data_t second;
    int64_t diff;

    updateTimesyncData(&first);
    delayMsec(5);
    updateTimesyncData(&second);

    EXPECT_TRUE(second.timestampInMsec > first.timestampInMsec);

    diff = second.timestampInMsec - first.timestampInMsec;
    EXPECT_GT(diff, 4);
    EXPECT_LT(diff, 8);

    EXPECT_TRUE(second.rtcTimestampInMsec > first.rtcTimestampInMsec);

    diff = second.rtcTimestampInMsec - first.rtcTimestampInMsec;
    EXPECT_GT(diff, 4);
    EXPECT_LT(diff, 8);
}
