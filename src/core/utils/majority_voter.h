#pragma once

#include <cstdint>

namespace teller::utils {

/**
 * @brief Majority voting algorithm for binary values.
 *
 * This class keeps track of the most recent 5 values of a single bit and
 * returns the majority of the recorded values.
 */
class MajorityVoter {
public:
    explicit MajorityVoter(bool initial = false)
    {
        reset(initial);
    }

    /** Feeds a new bit into the majority voter */
    void feed(bool bit)
    {
        history = ((history << 1) | (bit ? 1 : 0)) & MASK;
    }

    /**
     * Feeds a new bit into the majority voter and returns whether the vote
     * changed due to the new bit.
     */
    bool feedAndCheck(bool bit)
    {
        bool old = get();
        feed(bit);
        return old != get();
    }

    /** Returns the current majority value */
    bool get() const
    {
        return MAJORITY_TABLE & (1 << history);
    }

    /** Resets the voter to a state where it is assumed that the entire
     * history is filled with the given value. */
    void reset(bool value = false)
    {
        history = value ? MASK : 0;
    }

private:
    /** Internal state variable keeping track of the last bits */
    uint8_t history;

    /** Helper constant to mask the history variable */
    static const uint8_t MASK = (1 << 5) - 1;

    /** Helper constant where the i-th bit is set if the majority of the last five
     * recorded bits is true if the last five bits can be represented in binary
     * as the number i.
     *
     * Magic value for 3-majority is: 232
     * Magic value for 5-majority is: 4276676736
     *
     * Python one-liner to validate the values:
     *
     * \verbatim
     * int("".join("1" if x.bit_count() >= 2 else "0" for x in reversed(range(8))), base=2)
     * int("".join("1" if x.bit_count() >= 3 else "0" for x in reversed(range(32))), base=2)
     * \endverbatim
     */
    static const uint32_t MAJORITY_TABLE = 4276676736;
};

}
