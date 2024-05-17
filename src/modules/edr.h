#pragma once

#include <cstdlib>

namespace littlefs {
class Filesystem;
}

namespace teller::edr {

/**
 * Initializes the data structures required by the experiment data recorder module.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * Destroys the data structures required by the experiment data recorder module.
 */
void destroy(void);

/**
 * @brief Class that is responsible for recording experiment data into log files
 * on a filesystem.
 */
class ExperimentDataRecorder {

public:
    explicit ExperimentDataRecorder(littlefs::Filesystem* fs)
        : _fs(fs)
    {
    }
    void run();

private:
    littlefs::Filesystem* _fs;

    size_t getLastLogIndex();
    void updateLastLogIndex(size_t index);
};

}
