#include <ctime>
#include <gtest/gtest.h>

#include "hal/board.h"

using namespace teller::hal::board;

class BoardTest : public testing::Test {
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

TEST_F(BoardTest, getBoardTemperature)
{
    EXPECT_EQ(25.0f, getBoardTemperature());
}

TEST_F(BoardTest, getBoardVoltage)
{
    EXPECT_EQ(3.3f, getBoardVoltage());
}

TEST_F(BoardTest, getReasonOfLastReset)
{
    EXPECT_EQ(RESET_REASON_NORMAL, getReasonOfLastReset());
}
