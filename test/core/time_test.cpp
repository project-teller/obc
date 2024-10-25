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
    broken_down_time_t time;
    uint64_t timestamp;

    EXPECT_FALSE(utcTimeToMsec(nullptr, &timestamp));

    time.year = 1970;
    time.month = 1;
    time.day = 1;
    time.hour = 0;
    time.minute = 0;
    time.second = 15;
    time.millisecond = 0;
    EXPECT_TRUE(utcTimeToMsec(&time, &timestamp));
    EXPECT_EQ(15000, timestamp);

    time.millisecond = 250;
    EXPECT_TRUE(utcTimeToMsec(&time, &timestamp));
    EXPECT_EQ(15250, timestamp);

    time.year = 1983;
    time.month = 7;
    time.day = 29;
    time.hour = 12;
    time.minute = 34;
    time.second = 56;
    time.millisecond = 125;
    EXPECT_TRUE(utcTimeToMsec(&time, nullptr));
    EXPECT_TRUE(utcTimeToMsec(&time, &timestamp));
    EXPECT_EQ(428330096125, timestamp);

    time.year = 1900;
    time.month = 1;
    time.day = 1;
    time.hour = 0;
    time.minute = 0;
    time.second = 0;
    time.millisecond = 0;
    EXPECT_FALSE(utcTimeToMsec(&time, &timestamp));
}

TEST(TimeTest, utcMsecToTime)
{
    broken_down_time_t time;

    EXPECT_TRUE(utcMsecToTime(15000, &time));
    EXPECT_EQ(time.year, 1970);
    EXPECT_EQ(time.month, 1);
    EXPECT_EQ(time.day, 1);
    EXPECT_EQ(time.hour, 0);
    EXPECT_EQ(time.minute, 0);
    EXPECT_EQ(time.second, 15);
    EXPECT_EQ(time.millisecond, 0);

    EXPECT_TRUE(utcMsecToTime(15250, &time));
    EXPECT_EQ(time.year, 1970);
    EXPECT_EQ(time.month, 1);
    EXPECT_EQ(time.day, 1);
    EXPECT_EQ(time.hour, 0);
    EXPECT_EQ(time.minute, 0);
    EXPECT_EQ(time.second, 15);
    EXPECT_EQ(time.millisecond, 250);

    EXPECT_TRUE(utcMsecToTime(428330096125, nullptr));
    EXPECT_TRUE(utcMsecToTime(428330096125, &time));
    EXPECT_EQ(time.year, 1983);
    EXPECT_EQ(time.month, 7);
    EXPECT_EQ(time.day, 29);
    EXPECT_EQ(time.hour, 12);
    EXPECT_EQ(time.minute, 34);
    EXPECT_EQ(time.second, 56);
    EXPECT_EQ(time.millisecond, 125);
}
