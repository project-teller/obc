#include <cstring>
#include <gtest/gtest.h>

#include "hal/system.h"
#include "modules/errors.h"
#include "modules/messages.h"
#include "modules/rxsm.h"

using namespace teller::hal::system;
using namespace teller::rxsm;
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

TEST_F(MessagesTest, updateHeartbeatData)
{
    frames::heartbeat_data_t data;

    updateHeartbeatData(&data);
    EXPECT_EQ(teller::errors::NO_ERROR, data.error);
    EXPECT_FALSE(data.rxsmStatusBits.lo || data.rxsmStatusBits.sods || data.rxsmStatusBits.soe);

    /* Set an error code, check whether it goes through */
    teller::errors::setError(teller::errors::NOT_ENOUGH_MEMORY);
    updateHeartbeatData(&data);
    EXPECT_EQ(teller::errors::NOT_ENOUGH_MEMORY, data.error);
    EXPECT_FALSE(data.rxsmStatusBits.lo || data.rxsmStatusBits.sods || data.rxsmStatusBits.soe);

    teller::errors::clearAllErrors();
    updateHeartbeatData(&data);
    EXPECT_EQ(teller::errors::NO_ERROR, data.error);

    /* Set some RXSM signals, check whether it goes through */
    teller::rxsm::update(/* sods = */ true, false, false);
    teller::rxsm::update(/* sods = */ true, false, false);
    teller::rxsm::update(/* sods = */ true, false, false);
    teller::rxsm::update(/* sods = */ true, false, false);
    teller::rxsm::update(/* sods = */ true, false, false);
    updateHeartbeatData(&data);
    EXPECT_FALSE(data.rxsmStatusBits.lo || data.rxsmStatusBits.soe);
    EXPECT_TRUE(data.rxsmStatusBits.sods);
}
