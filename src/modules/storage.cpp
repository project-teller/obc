#include <iostream>
#include <memory>

#include "hal/storage.h"
#include "littlefs-cpp.h"
#include "modules/storage.h"

using namespace teller::hal::storage;
using namespace teller::telem;

static const size_t NUM_AREAS = area::NUMBER_OF_AREAS;

/**
 * @brief Tracks the state of a single filesystem in the storage module.
 */
class FilesystemState {

    enum Flags {
        NO_FLAGS = 0,
        FLAG_MOUNTED = 1,
        FLAG_ERRORED = 2
    };

public:
    FilesystemState(littlefs::FilesystemConfig& cfg)
        : _fs(cfg)
        , _flags(NO_FLAGS)
    {
    }

    ~FilesystemState()
    {
        ensureUnmounted();
    }

    bool isErrored() const { return _flags & FLAG_ERRORED; }
    bool isMounted() const { return _flags & FLAG_MOUNTED; }

    /**
     * @brief Mounts the filesystem if it is not mounted yet.
     *
     * @return true if the filesystem is mounted after returning from the
     *         function call, false otherwise
     */
    bool ensureMounted()
    {
        if (isMounted()) {
            return true;
        }

        if (isErrored() || _fs.mount()) {
            return false;
        } else {
            _flags |= FLAG_MOUNTED;
            return true;
        }
    }

    /**
     * @brief Unmounts the filesystem if it is mounted.
     *
     * @return true if the filesystem is unmounted after returning from the
     *         function call, false otherwise
     */
    bool ensureUnmounted()
    {
        if (!isMounted()) {
            return true;
        }

        if (_fs.unmount()) {
            return false;
        } else {
            _flags &= ~FLAG_MOUNTED;
            return true;
        }
    }

    /**
     * @brief Marks the filesystem as having encountered an error.
     *
     * Filesystems with errors are unmounted and no attempts will be made to
     * mount them again until the error flag is cleared.
     */
    void markErrored()
    {
        _flags |= FLAG_ERRORED;
    }

    /**
     * @brief Clears the error flag on the filesystem.
     */
    void clearErrors()
    {
        _flags &= ~FLAG_ERRORED;
    }

    /**
     * @brief Conversion operator to allow the state object to be used directly
     * in LittleFS functions.
     */
    operator littlefs::Filesystem*()
    {
        return &_fs;
    }

private:
    littlefs::Filesystem _fs;
    uint8_t _flags;

    friend class Filesystems;
};

class Filesystems {
public:
    Filesystems() { }

    ~Filesystems()
    {
        destroy();
    }

    Filesystems(const Filesystems&) = delete;
    Filesystems& operator=(const Filesystems&) = delete;

    bool init()
    {
        for (size_t i = 0; i < NUM_AREAS; i++) {
            area::area_t area = static_cast<area::area_t>(i);
            auto cfg = getFilesystemConfig(area);
            if (cfg) {
                _filesystems[i] = std::make_shared<FilesystemState>(*cfg);
            } else {
                _filesystems[i].reset();
            }
        }

        return true;
    }

    void destroy()
    {
        for (size_t i = 0; i < NUM_AREAS; i++) {
            _filesystems[i].reset();
        }
    }

    /**
     * @brief Returns whether all filesystems that are defined are also mounted
     * and are not marked as erroneous.
     */
    bool allMounted() const
    {
        for (size_t i = 0; i < NUM_AREAS; i++) {
            if (_filesystems[i] && (!_filesystems[i]->isMounted() || _filesystems[i]->isErrored())) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Returns whether at least one filesystem is marked as erroneous.
     */
    bool anyErrored() const
    {
        for (size_t i = 0; i < NUM_AREAS; i++) {
            if (_filesystems[i] && _filesystems[i]->isErrored()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Returns whether at least one filesystem that is defined is also
     * mounted and is not marked as erroneous.
     */
    bool anyMounted() const
    {
        for (size_t i = 0; i < NUM_AREAS; i++) {
            if (_filesystems[i] && _filesystems[i]->isMounted() && !_filesystems[i]->isErrored()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Clears the error flag on all filesystems.
     */
    void clearErrors()
    {
        for (size_t i = 0; i < NUM_AREAS; i++) {
            if (_filesystems[i]) {
                _filesystems[i]->clearErrors();
            }
        }
    }

    /**
     * @brief Returns a pointer to the filesystem of the given storage area.
     *
     * @param area  the identifier of the storage area
     * @param ensureMounted  whether to ensure that the filesystem is mounted
     * @return  the pointer to the filesystem. Returns null if the area has no
     *          registered filesystem or when an error happened while mounting
     */
    littlefs::Filesystem* get(area::area_t area, bool ensureMounted = true)
    {
        auto state = _filesystems[area];
        if (state) {
            if (ensureMounted) {
                if (!state->ensureMounted()) {
                    return nullptr;
                }
            }
            return &state->_fs;
        } else {
            return nullptr;
        }
    }

    /**
     * @brief Attempts to mount all filesystems.
     */
    bool mountAll()
    {
        bool success = true;

        for (size_t i = 0; i < NUM_AREAS; i++) {
            if (_filesystems[i]) {
                if (!_filesystems[i]->ensureMounted()) {
                    success = false;
                }
            }
        }

        return success;
    }

    littlefs::Filesystem* operator[](area::area_t area)
    {
        return get(area, /* ensureMounted = */ false);
    }

private:
    std::shared_ptr<FilesystemState> _filesystems[NUM_AREAS];
};

Filesystems fs;

namespace teller::storage {

bool init()
{
    std::optional<littlefs::Error> err;

    if (!fs.init()) {
        return false;
    }

    fs.mountAll();

    return true;
}

void destroy()
{
    fs.destroy();
}

subsystem_status_t getSubsystemStatus()
{
    if (fs.anyErrored()) {
        /* At least one filesystem has an error */
        return SUBSYSTEM_STATUS_ERROR;
    } else if (fs.allMounted()) {
        /* All filesystems that should be mounted are mounted and have no errors */
        return SUBSYSTEM_STATUS_OK;
    } else if (fs.anyMounted()) {
        /* Not all filesystems are mounted, but some are */
        return SUBSYSTEM_STATUS_WARNING;
    } else {
        /* No filesystems are mounted at all, even though there are some */
        return SUBSYSTEM_STATUS_CRITICAL;
    }
}

}
