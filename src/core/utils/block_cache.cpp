#include "core/utils/block_cache.h"

using namespace std;
using namespace teller::utils;

BlockCache::BlockCache(std::size_t blockSize_)
    : blockSize(blockSize_)
    , blockIndex(0)
    , isValid(false)
    , hitCounter(0)
    , missCounter(0)
{
    this->block = new uint8_t[blockSize_];
}

BlockCache::~BlockCache()
{
    delete[] this->block;
}

void BlockCache::clear()
{
    this->hitCounter = 0;
    this->missCounter = 0;
    this->isValid = false;
}

const uint8_t* BlockCache::commit(BlockIndex blockIndex_)
{
    this->blockIndex = blockIndex_;
    this->isValid = true;
    return this->block;
}

void BlockCache::evict(BlockIndex blockIndex_)
{
    if (this->blockIndex == blockIndex_) {
        this->isValid = false;
    }
}

void BlockCache::getCounters(uint32_t& hit, uint32_t& miss) const
{
    hit = this->hitCounter;
    miss = this->missCounter;
}

uint8_t* BlockCache::getScratchArea()
{
    this->isValid = false;
    return block;
}
