#include "core/utils/block_cache.h"

using namespace std;
using namespace teller::utils;

BlockCache::BlockCache(std::size_t blockSize_)
    : blockSize(blockSize)
    , blockIndex(0)
    , isValid(false)
{
    this->block = new uint8_t[blockSize_];
}

BlockCache::~BlockCache()
{
    delete this->block;
}

void BlockCache::clear()
{
    this->isValid = false;
}

void BlockCache::commit(BlockIndex blockIndex_)
{
    this->blockIndex = blockIndex_;
    isValid = true;
}

uint8_t* BlockCache::evict()
{
    this->isValid = false;
    return block;
}

uint8_t* BlockCache::getScratchArea()
{
    return evict();
}
