#include <gtest/gtest.h>

#include "core/math/running_mean.hpp"

using namespace teller::math;

TEST(RunningMeanTest, simple)
{
    RunningMean<float> mean;

    EXPECT_EQ(mean.get(), 0);
    EXPECT_EQ(mean.countSamples(), 0);

    mean.add(5);
    EXPECT_EQ(mean.get(), 5);
    EXPECT_EQ(mean.countSamples(), 1);

    mean.add(10);
    EXPECT_EQ(mean.get(), 7.5f);
    EXPECT_EQ(mean.countSamples(), 2);

    mean.add(2.5);
    mean.add(2.5);
    EXPECT_EQ(mean.get(), 5.0f);
    EXPECT_EQ(mean.countSamples(), 4);

    mean.reset();
    EXPECT_EQ(mean.get(), 0);
    EXPECT_EQ(mean.countSamples(), 0);
}
