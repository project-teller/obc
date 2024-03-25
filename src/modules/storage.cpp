#include <cerrno>
#include <iostream>
#include <memory>

#include "hal/storage.h"
#include "modules/storage.h"

using namespace teller::hal::storage;
using namespace teller::telem;

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

    std::optional<littlefs::Error> format()
    {
        return _fs.format();
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
        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
            storage_area_t area = static_cast<storage_area_t>(i);
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
        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
            _filesystems[i].reset();
        }
    }

    /**
     * @brief Returns whether all filesystems that are defined are also mounted
     * and are not marked as erroneous.
     */
    bool allMounted() const
    {
        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
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
        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
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
        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
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
        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
            if (_filesystems[i]) {
                _filesystems[i]->clearErrors();
            }
        }
    }

    /**
     * @brief Returns a pointer to the state of the filesystem of the given storage area.
     *
     * @param area  the identifier of the storage area
     * @param ensureMounted  whether to ensure that the filesystem is mounted
     * @return  the pointer to the state of the filesystem. Returns null if the
     *          area has no registered filesystem or when an error happened while
     *          mounting
     */
    std::shared_ptr<FilesystemState> getState(storage_area_t area, bool ensureMounted = true)
    {
        auto state = area >= 0 && area < NUM_STORAGE_AREAS ? _filesystems[area] : nullptr;
        if (state) {
            if (ensureMounted) {
                if (!state->ensureMounted()) {
                    return nullptr;
                }
            }
            return state;
        } else {
            return nullptr;
        }
    }

    /**
     * @brief Attempts to format all filesystems, unmounting them as needed.
     */
    bool formatAll()
    {
        bool success = true;

        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
            auto fs = _filesystems[i];
            if (fs) {
                if (!fs->ensureUnmounted() || fs->_fs.format()) {
                    success = false;
                }
            }
        }

        return success;
    }

    /**
     * @brief Attempts to mount all filesystems.
     */
    bool mountAll()
    {
        bool success = true;

        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
            if (_filesystems[i]) {
                if (!_filesystems[i]->ensureMounted()) {
                    success = false;
                }
            }
        }

        return success;
    }

    littlefs::Filesystem* operator[](storage_area_t area)
    {
        auto state = getState(area, /* ensureMounted = */ false);
        return state ? &state->_fs : nullptr;
    }

private:
    std::shared_ptr<FilesystemState> _filesystems[NUM_STORAGE_AREAS];
};

Filesystems fs;

namespace teller::storage {

bool init(bool format)
{
    std::optional<littlefs::Error> err;

    if (!fs.init()) {
        return false;
    }

    if (format) {
        fs.formatAll();
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

int eraseStorage(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem) {
        return EINVAL;
    }

    if (_filesystem->isMounted()) {
        return EIO;
    }

    auto error_code = _filesystem->format();
    if (error_code) {
        _filesystem->markErrored();
        return convertLittleFSErrorCode(error_code);
    } else {
        return 0;
    }
}

bool isStorageMounted(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    return _filesystem && _filesystem->isMounted();
}

void markStorageAsErrored(teller::telem::storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (_filesystem) {
        _filesystem->markErrored();
    }
}

int mountStorage(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem) {
        return EINVAL;
    }

    if (!_filesystem->ensureMounted()) {
        _filesystem->markErrored();
        return EIO;
    }

    return 0;
}

int unmountStorage(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem) {
        return EINVAL;
    }

    if (!_filesystem->ensureUnmounted()) {
        _filesystem->markErrored();
        return EIO;
    }

    return 0;
}

int convertLittleFSErrorCode(std::optional<littlefs::Error> code)
{
    if (!code) {
        return 0;
    }

    switch (*code) {
    case littlefs::Error::BADF:
        return EBADF;

    case littlefs::Error::EXIST:
        return EEXIST;

    case littlefs::Error::FBIG:
        return EFBIG;

    case littlefs::Error::INVAL:
        return EINVAL;

    case littlefs::Error::IO:
        return EIO;

    case littlefs::Error::ISDIR:
        return EISDIR;

    case littlefs::Error::NAMETOOLONG:
        return ENAMETOOLONG;

    case littlefs::Error::NO_DD_ENTRY:
    case littlefs::Error::NO_FD_ENTRY:
    case littlefs::Error::NOENT:
        return ENOENT;

    case littlefs::Error::NOMEM:
        return ENOMEM;

    case littlefs::Error::NOSPC:
        return ENOSPC;

    case littlefs::Error::NOTDIR:
        return ENOTDIR;

    case littlefs::Error::NOTEMPTY:
        return ENOTEMPTY;

    default:
        return EIO;
    }
}

}
