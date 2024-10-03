#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/block_cache.h"

using namespace teller::utils;

void expectCounters(BlockCache& cache, uint8_t hit_, uint8_t miss_)
{
    uint32_t hit, miss;

    cache.getCounters(hit, miss);
    EXPECT_EQ(hit_, hit);
    EXPECT_EQ(miss_, miss);
}

TEST(BlockCacheTest, initialState)
{
    BlockCache cache(8);
    expectCounters(cache, 0, 0);
}

TEST(BlockCacheTest, readWriteCycle)
{
    BlockCache cache(4);
    uint8_t* buf;
    const uint8_t* constBuf;

    EXPECT_EQ(nullptr, cache.getBlock(17));
    expectCounters(cache, 0, 1);

    buf = cache.getScratchArea();
    EXPECT_NE(nullptr, buf);
    EXPECT_NE(nullptr, cache.commit(17));

    constBuf = cache.getBlock(17);
    EXPECT_NE(nullptr, constBuf);
    expectCounters(cache, 1, 1);

    EXPECT_EQ(constBuf, cache.getBlock(17));
    expectCounters(cache, 2, 1);

    EXPECT_EQ(constBuf, cache.getBlock(17));
    expectCounters(cache, 3, 1);

    EXPECT_EQ(nullptr, cache.getBlock(18));
    expectCounters(cache, 3, 2);

    cache.clear();
    expectCounters(cache, 0, 0);

    EXPECT_EQ(nullptr, cache.getBlock(17));
    EXPECT_EQ(nullptr, cache.getBlock(18));
    expectCounters(cache, 0, 2);
}

TEST(BlockCacheTest, evict)
{
    BlockCache cache(4);
    uint8_t* buf;

    buf = cache.getScratchArea();
    EXPECT_NE(nullptr, buf);
    EXPECT_NE(nullptr, cache.commit(17));

    EXPECT_NE(nullptr, cache.getBlock(17));
    expectCounters(cache, 1, 0);

    cache.evict(18);
    EXPECT_NE(nullptr, cache.getBlock(17));
    expectCounters(cache, 2, 0);

    cache.evict(17);
    EXPECT_EQ(nullptr, cache.getBlock(17));
    expectCounters(cache, 2, 1);
}
