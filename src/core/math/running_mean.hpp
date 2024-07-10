#pragma once

namespace teller::math {

/**
 * @brief Running mean calculator using floats or doubles.
 */
template <typename T>
class RunningMean {
public:
    explicit RunningMean()
    {
        reset();
    }

    /** Adds a new sample to the running mean */
    void add(T sample)
    {
        numSamples++;
        current += (sample - current) / numSamples;
    }

    /** Returns how many samples are used in the current mean */
    int countSamples() const
    {
        return numSamples;
    }

    /** Returns the current mean */
    T get() const
    {
        return current;
    }

    /** Resets the running mean calculator to no samples and zero mean */
    void reset()
    {
        numSamples = 0;
        current = 0;
    }

private:
    int numSamples;
    T current;
};

}
