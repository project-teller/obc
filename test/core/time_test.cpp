#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/time.h"

TEST(TimeTest, timespecDiff)
{
    struct timespec first = { .tv_sec = 1234, .tv_nsec = 5678 };
    struct timespec second = { .tv_sec = 5678, .tv_nsec = 1234 };
    struct timespec third = { .tv_sec = 5678, .tv_nsec = 6789 };
    struct timespec diff;

    timespecDiff(&first, &second, &diff);

    EXPECT_EQ(4443, diff.tv_sec);
    EXPECT_EQ(999995556, diff.tv_nsec);

    timespecDiff(&first, &third, &diff);

    EXPECT_EQ(4444, diff.tv_sec);
    EXPECT_EQ(1111, diff.tv_nsec);
}

TEST(TimeTest, timespecToMsec)
{
    struct timespec spec = { .tv_sec = 1234, .tv_nsec = 512000000 };
    struct timespec spec_negative = { .tv_sec = -1234, .tv_nsec = 512000000 };

    EXPECT_EQ(1234512, timespecToMsec(&spec));
    EXPECT_EQ(-1233488, timespecToMsec(&spec_negative));
}

TEST(TimeTest, utcTimeToMsec)
{
    EXPECT_EQ(15000, utcTimeToMsec(1970, 1, 1, 0, 0, 15, 0));
    EXPECT_EQ(15250, utcTimeToMsec(1970, 1, 1, 0, 0, 15, 250));
    EXPECT_EQ(428330096125, utcTimeToMsec(1983, 7, 29, 12, 34, 56, 125));
    EXPECT_EQ(0, utcTimeToMsec(1900, 1, 1, 0, 0, 0, 0));
}
