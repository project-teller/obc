#include <cerrno>
#include <iostream>
#include <memory>

#include "core/telem/binary_data.h"
#include "drivers/flashmem.h"
#include "drivers/sdcard.h"
#include "hal/event_flags.hpp"
#include "hal/system.h"
#include "modules/log.h"
#include "modules/storage.h"
#include "modules/telem.h"

using namespace teller::drivers;
using namespace teller::hal::system;
using namespace teller::telem;

static teller::log::Logger* logger = nullptr;

/**
 * @brief Returns whether the given storage area is expected to be present.
 */
bool isStorageAreaMandatory(size_t area)
{
    return area == STORAGE_AREA_FLASH_MEMORY || area == STORAGE_AREA_SD_CARD;
}

/**
 * @brief Returns whether the given storage area should be mounted at boot.
 */
bool shouldMountStorageAreaAtBoot(size_t area)
{
    return area == STORAGE_AREA_FLASH_MEMORY || area == STORAGE_AREA_SD_CARD;
}

/**
 * @brief Tracks the state of a single filesystem in the storage module.
 */
class FilesystemState {

    enum Events {
        EVT_MOUNTED = 1,
        EVT_UNMOUNTED = 2,
    };

    enum Flags {
        NO_FLAGS = 0,
        FLAG_CONFIGURED = 1,
        FLAG_MOUNTED = 2,
        FLAG_ERRORED = 4
    };

public:
    FilesystemState(storage_area_t area)
        : _area(area)
        , _fs()
        , _flags(NO_FLAGS)
    {
    }

    ~FilesystemState()
    {
        ensureUnmounted();
        if (_fs) {
            delete _fs;
        }
    }

    bool isConfigured() const { return _flags & FLAG_CONFIGURED; }
    bool isErrored() const { return _flags & FLAG_ERRORED; }
    bool isMounted() const { return _flags & FLAG_MOUNTED; }

    /**
     * @brief Configures the filesystem with the given LittleFS configuration object.
     *
     * This can be called once and only once, before the filesystem is mounted.
     */
    bool configure(littlefs::FilesystemConfig& cfg)
    {
        if (isConfigured()) {
            return false;
        }

        _fs = new littlefs::Filesystem(cfg);
        if (!_fs) {
            return false;
        }

        _flags |= FLAG_CONFIGURED;
        return true;
    }

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

        if (!_fs) {
            return false;
        }

        if (isErrored() || _fs->mount()) {
            return false;
        } else {
            _flags |= FLAG_MOUNTED;
            _events.set(EVT_MOUNTED);
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

        if (!_fs) {
            return true;
        }

        if (_fs->unmount()) {
            return false;
        } else {
            _flags &= ~FLAG_MOUNTED;
            _events.set(EVT_UNMOUNTED);
            return true;
        }
    }

    /**
     * @brief Formats the filesystem.
     */
    std::optional<littlefs::Error> format()
    {
        if (_fs) {
            return _fs->format();
        } else {
            return littlefs::Error::INVAL;
        }
    }

    /**
     * @brief Returns the current operation that the filesystem is performing.
     */
    teller::drivers::StorageOperation getCurrentStorageOperation()
    {
        if (!isConfigured()) {
            return OP_UNCONFIGURED;
        } else if (!isMounted()) {
            return OP_UNMOUNTED;
        } else if (isErrored()) {
            return OP_ERROR;
        } else {
            switch (_area) {
            case STORAGE_AREA_FLASH_MEMORY:
                return teller::drivers::flashmem::getCurrentOperation();
            case STORAGE_AREA_SD_CARD:
                return teller::drivers::sdcard::getCurrentOperation();
            default:
                return OP_UNKNOWN;
            }
        }
    }

    /**
     * @brief Returns the storage statistics of the filesystem.
     */
    teller::drivers::StorageStatistics getStatistics()
    {
        static const StorageStatistics ZERO = {};

        switch (_area) {
        case STORAGE_AREA_FLASH_MEMORY:
            return teller::drivers::flashmem::getStatistics();
        case STORAGE_AREA_SD_CARD:
            return teller::drivers::sdcard::getStatistics();
        default:
            return ZERO;
        }
    }

    /**
     * @brief Returns the size of the filesystem, in bytes.
     *
     * @returns The size of the filesystem, or zero if the size cannot be
     *          determined.
     */
    size_t getSize()
    {
        switch (_area) {
        case STORAGE_AREA_FLASH_MEMORY:
            return teller::drivers::flashmem::getTotalSize();
        case STORAGE_AREA_SD_CARD:
            return teller::drivers::sdcard::getTotalSize();
        default:
            return 0;
        }
    }

    /**
     * @brief Marks the filesystem as having encountered an error.
     *
     * Filesystems with errors are unmounted and no attempts will be made to
     * mount them again until the error flag is cleared.
     *
     * @param  error  a POSIX error code to report in the log
     * @return the same error code as the one that was received
     */
    int markErrored(int error = 0)
    {
        if (!(_flags & FLAG_ERRORED)) {
            if (logger) {
                if (error) {
                    logger->error(
                        "%s: %s (errno=%d)", getStorageAreaName(_area),
                        strerror(error), error);
                } else {
                    logger->error("%s: marked as errored", getStorageAreaName(_area));
                }
            }

            _flags |= FLAG_ERRORED;
        }

        return error;
    }

    /**
     * @brief Clears the error flag on the filesystem.
     */
    void clearErrors()
    {
        _flags &= ~FLAG_ERRORED;
    }

    /**
     * @brief Reads the raw contents of the storage underneath the filesystem.
     *
     * @param buf  the buffer to read the data into
     * @param address  the address to read from
     * @param length  the number of bytes to read
     */
    bool readRawData(uint8_t* buf, uint64_t address, size_t length)
    {
        switch (_area) {
        case STORAGE_AREA_FLASH_MEMORY:
            return teller::drivers::flashmem::readData(buf, address, length);

        case STORAGE_AREA_SD_CARD:
            return teller::drivers::sdcard::readData(buf, address, length);

        default:
            return false;
        }
    }

    /**
     * @brief Waits until the filesystem becomes mounted.
     *
     * Returns immediately if the filesystem is already mounted.
     */
    littlefs::Filesystem* waitUntilMounted()
    {
        while (!isMounted()) {
            _events.waitAny(EVT_MOUNTED);
        }
        assert(_fs != nullptr);
        return _fs;
    }

    /**
     * @brief Waits until the filesystem becomes unmounted.
     *
     * Returns immediately if the filesystem is not mounted.
     */
    littlefs::Filesystem* waitUntilUnmounted()
    {
        while (isMounted()) {
            _events.waitAny(EVT_UNMOUNTED);
        }
        assert(_fs != nullptr);
        return _fs;
    }

    /**
     * @brief Conversion operator to allow the state object to be used directly
     * in LittleFS functions.
     */
    operator littlefs::Filesystem*()
    {
        return _fs;
    }

private:
    storage_area_t _area;
    littlefs::Filesystem* _fs;
    uint8_t _flags;
    teller::hal::EventFlags _events;

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
            _filesystems[i] = i == STORAGE_AREA_UNKNOWN ? nullptr : std::make_shared<FilesystemState>(area);
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
            if (_filesystems[i]) {
                if (!_filesystems[i]->isMounted() || _filesystems[i]->isErrored()) {
                    return false;
                }
            } else {
                if (isStorageAreaMandatory(i)) {
                    return false;
                }
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
                if (!fs->ensureUnmounted() || !fs->_fs || fs->_fs->format()) {
                    success = false;
                }
            }
        }

        return success;
    }

    /**
     * @brief Attempts to format filesystems that failed to mount at boot but
     * should be mounted.
     */
    bool formatAtBoot()
    {
        bool success = true;

        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
            auto fs = _filesystems[i];
            if (fs && !fs->isMounted() && shouldMountStorageAreaAtBoot(i) && fs->_fs) {
                if (fs->_fs->format().has_value()) {
                    success = false;
                }
            }
        }

        return success;
    }

    /**
     * @brief Attempts to mount the filesystems that should be mounted at boot.
     */
    bool mountAtBoot()
    {
        bool success = true;

        for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
            if (_filesystems[i] && shouldMountStorageAreaAtBoot(i)) {
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
        return state ? state->_fs : nullptr;
    }

private:
    std::shared_ptr<FilesystemState> _filesystems[NUM_STORAGE_AREAS];
};

static Filesystems fs;

class StorageReaderState {
public:
    enum Events {
        EVT_STARTED = 1,
    };

    void setTargets(uint8_t targets)
    {
        _targets = targets;
    }

    bool startReading(storage_area_t area, uint64_t address, uint16_t length, uint8_t seq_no)
    {
        if (running() || length == 0) {
            return false;
        }

        _area = area;
        _address = address;
        _bytesLeft = length;

        memset(&_binaryData, 0, sizeof(_binaryData));

        _binaryData.frame_type = frames::STORAGE;
        _binaryData.seq_no = seq_no;
        _binaryData.fragment_index = 0;
        _binaryData.max_fragment_index = (length / MAX_BINARY_DATA_FRAGMENT_LENGTH);
        if (length % MAX_BINARY_DATA_FRAGMENT_LENGTH == 0) {
            _binaryData.max_fragment_index--;
        }

        _events.set(EVT_STARTED);

        return true;
    }

    bool running() const
    {
        return _bytesLeft > 0;
    }

    bool iterate()
    {
        const uint8_t limit = MAX_BINARY_DATA_FRAGMENT_LENGTH;

        while (!running()) {
            _events.waitAny(EVT_STARTED);
        }

        auto _filesystem = fs.getState(_area, /* ensureMounted = */ false);
        if (!_filesystem) {
            return EINVAL;
        }

        while (running()) {
            _binaryData.data_length = _bytesLeft > limit ? limit : _bytesLeft;
            if (!_filesystem->readRawData(_binaryData.data, _address, _binaryData.data_length)) {
                break;
            }

            uint8_t length = frames::encodeBinaryDataFrame(&_binaryData, _buf);
            if (!teller::telem::sendTo(_targets, frames::BINARY_DATA, _buf, length)) {
                return false;
            }

            _address += _binaryData.data_length;
            _bytesLeft -= _binaryData.data_length;
            _binaryData.fragment_index++;
        }

        return _bytesLeft == 0;
    }

private:
    storage_area_t _area;
    uint64_t _address;
    uint16_t _bytesLeft;
    teller::hal::EventFlags _events;
    frames::binary_data_t _binaryData;
    uint8_t _buf[MAX_PAYLOAD_LENGTH];
    uint8_t _targets;
};

static StorageReaderState storageReader;

namespace teller::storage {

bool init()
{
    logger = teller::log::getLogger(MODULE_ID_EDR);
    if (!logger) {
        return false;
    }

    if (!fs.init()) {
        logger = nullptr;
        return false;
    }

    return true;
}

void setup(InitMode mode)
{
    std::optional<littlefs::Error> err;
    bool success;

    {
        auto cfg = teller::drivers::flashmem::setup();
        if (cfg) {
            configureStorage(STORAGE_AREA_FLASH_MEMORY, *cfg);
        }
    }

    {
        auto cfg = teller::drivers::sdcard::setup();
        if (cfg) {
            configureStorage(STORAGE_AREA_SD_CARD, *cfg);
        }
    }

    if (mode == INIT_MODE_FORMAT) {
        if (!fs.formatAll()) {
            return;
        }
    }

    success = fs.mountAtBoot();

    if (!success && mode == INIT_MODE_FORMAT_IF_NEEDED) {
        fs.formatAtBoot();
        fs.mountAtBoot();
    }
}

void destroy()
{
    fs.destroy();
    teller::drivers::sdcard::destroy();
    teller::drivers::flashmem::destroy();
    logger = nullptr;
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

bool configureStorage(storage_area_t area, littlefs::FilesystemConfig& cfg)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem || _filesystem->isConfigured()) {
        return false;
    } else {
        return _filesystem->configure(cfg);
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
        return _filesystem->markErrored(convertLittleFSErrorCode(error_code));
    } else {
        return 0;
    }
}

bool isStorageConfigured(teller::telem::storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    return _filesystem && _filesystem->isConfigured();
}

bool isStorageErrored(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    return _filesystem && _filesystem->isErrored();
}

bool isStorageMounted(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    return _filesystem && _filesystem->isMounted();
}

void markStorageAsErrored(teller::telem::storage_area_t area, int error)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (_filesystem) {
        _filesystem->markErrored(error);
    }
}

int mountStorage(storage_area_t area, bool force)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem) {
        return EINVAL;
    }

    if (force) {
        _filesystem->clearErrors();
    }

    if (!_filesystem->ensureMounted()) {
        return _filesystem->markErrored(EIO);
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
        return _filesystem->markErrored(EIO);
    }

    return 0;
}

littlefs::Filesystem* waitUntilMounted(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem) {
        sleepForever();
        return nullptr;
    } else {
        return _filesystem->waitUntilMounted();
    }
}

littlefs::Filesystem* waitUntilUnmounted(storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem) {
        sleepForever();
        return nullptr;
    } else {
        return _filesystem->waitUntilUnmounted();
    }
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

StorageOperation getCurrentStorageOperation(teller::telem::storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    if (!_filesystem) {
        return OP_UNCONFIGURED;
    } else {
        return _filesystem->getCurrentStorageOperation();
    }
}

int getStorageSize(teller::telem::storage_area_t area)
{
    auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
    return _filesystem ? _filesystem->getSize() : 0;
}

void reportStatus(void)
{
    for (size_t i = 0; i < NUM_STORAGE_AREAS; i++) {
        teller::telem::storage_area_t area = static_cast<teller::telem::storage_area_t>(i);
        StorageStatistics stats;

        auto _filesystem = fs.getState(area, /* ensureMounted = */ false);
        const char* opStr;

        if (!_filesystem) {
            continue;
        }

        stats = _filesystem->getStatistics();

        switch (_filesystem->getCurrentStorageOperation()) {
        case OP_ERASE:
            opStr = "erasing";
            break;
        case OP_ERROR:
            opStr = "error";
            break;
        case OP_IDLE:
            opStr = "idle";
            break;
        case OP_READ:
            opStr = "reading";
            break;
        case OP_SYNC:
            opStr = "syncing";
            break;
        case OP_UNCONFIGURED:
            opStr = "unconfigured";
            break;
        case OP_UNMOUNTED:
            opStr = "unmounted";
            break;
        case OP_WRITE:
            opStr = "writing";
            break;
        case OP_UNKNOWN:
        default:
            opStr = "unknown";
        }

        logger->info_nowait(
            "%s: %s, rd: %lu, wr: %lu, e: %lu, re: %lu",
            teller::telem::getStorageAreaName(area),
            opStr,
            stats.bytesRead,
            stats.bytesWritten,
            stats.blocksErased,
            stats.retries);
    }
}

int startReadingStorage(
    teller::telem::storage_area_t area, uint64_t address, uint16_t length,
    uint8_t targets, uint8_t seq_no)
{
    if (!storageReader.running()) {
        storageReader.setTargets(targets);
        storageReader.startReading(area, address, length, seq_no);
        return 0;
    } else {
        return EBUSY;
    }
}

void runStorageReader()
{
    while (true) {
        while (storageReader.iterate()) { };
    }
}

}
