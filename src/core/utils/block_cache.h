#pragma once

#include <cstdint>
#include <cstdlib>

namespace teller::utils {

/**
 * @brief Size-1 block cache for filesystem operations.
 */
class BlockCache {
public:
    typedef std::size_t BlockIndex;

    explicit BlockCache(std::size_t blockSize_);
    ~BlockCache();

    /**
     * @brief Clears the cache.
     */
    void clear();

    /**
     * @brief Commits the scratchpad area to the cache as a new block.
     *
     * @param   blockIndex_  index of the block that the scratchpad area stores
     * @return  pointer to the block in the cache
     */
    const uint8_t* commit(BlockIndex blockIndex_);

    /**
     * @brief Evicts a block from the cache.
     */
    void evict();

    /**
     * @brief Returns the block with the given index if it is in the cache, null otherwise.
     *
     * @param blockIndex_  index of the block to retrieve
     * @return pointer to the block if it is in the cache, null otherwise
     */
    const uint8_t* getBlock(BlockIndex blockIndex_) const
    {
        if (isValid && blockIndex_ == blockIndex) {
            this->hitCounter++;
            return this->block;
        } else {
            this->missCounter++;
            return nullptr;
        }
    }

    /**
     * @brief Returns the hit and miss counters of the cache for read operations.
     */
    void getCounters(uint32_t& hit, uint32_t& miss) const;

    /**
     * @brief Returns a "scratchpad" memory area where a new block can be stored.
     *
     * May evict a block from the cache if needed, under the assumption
     * that we need to call this function only if we are about to store a new
     * block in the cache.
     */
    uint8_t* getScratchArea();

    /**
     * @brief Evicts the block with the given index from the cache if it is cached.
     *
     * No-op if the block is not in the cache.
     */
    void evict(BlockIndex blockIndex_);

private:
    /** The size of each block in the cache */
    std::size_t blockSize;

    /** Index of the currently cached block */
    BlockIndex blockIndex;

    /** Memory area holding the cached block */
    uint8_t* block;

    /**
     * Stores whether there is a block in the cache (i.e. whether the block
     * index is valid.
     */
    bool isValid;

    /** Counter of cache hits */
    mutable uint32_t hitCounter;

    /** Counter of cache missees */
    mutable uint32_t missCounter;
};

}
