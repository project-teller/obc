#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/majority_voter.h"

using namespace teller::utils;

TEST(MajorityVoterTest, initialization)
{
    MajorityVoter voter;

    EXPECT_FALSE(voter.get());

    voter.feed(true);
    EXPECT_FALSE(voter.get());

    voter.feed(true);
    EXPECT_FALSE(voter.get());

    voter.feed(true);
    EXPECT_TRUE(voter.get());

    voter.reset(true);

    EXPECT_TRUE(voter.get());

    voter.feed(false);
    EXPECT_TRUE(voter.get());

    voter.feed(false);
    EXPECT_TRUE(voter.get());

    voter.feed(false);
    EXPECT_FALSE(voter.get());
}

TEST(MajorityVoterTest, feedAndGet)
{
    MajorityVoter voter;
    /* clang-format off */
    /* This is a (2, 5) de Bruijn sequence. It contains all possible 5-bit
     * substrings while being minimal in length */
    int sequence[] = {
        0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0,
        1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0
    };
    int expected[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0
    };
    /* clang-format on */
    constexpr size_t n = sizeof(sequence) / sizeof(sequence[0]);
    static_assert(sizeof(expected) / sizeof(expected[0]) == n);

    for (size_t i = 0; i < n; i++) {
        voter.feed(sequence[i]);
        EXPECT_EQ(expected[i], voter.get() ? 1 : 0) << "Failure at index " << i;
    }
}

TEST(MajorityVoterTest, feedAndCheck)
{
    MajorityVoter voter;
    /* clang-format off */
    /* This is a (2, 5) de Bruijn sequence. It contains all possible 5-bit
     * substrings while being minimal in length */
    int sequence[] = {
        0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0,
        1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0
    };
    int expected[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0
    };
    /* clang-format on */
    constexpr size_t n = sizeof(sequence) / sizeof(sequence[0]);
    static_assert(sizeof(expected) / sizeof(expected[0]) == n);

    for (size_t i = 0; i < n; i++) {
        EXPECT_EQ(expected[i], voter.feedAndCheck(sequence[i])) << "Failure at index " << i;
    }
}
