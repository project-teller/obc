#include <cerrno>
#include <iostream>
#include <memory>

#include "core/telem/binary_data.h"
#include "core/telem/directory_entry.h"
#include "core/utils/smart_dir_handle.h"
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
     * @brief Returns a handle to the filesystem if it is mounted, null otherwise.
     */
    littlefs::Filesystem* getFilesystemIfMounted()
    {
        return isMounted() ? _fs : nullptr;
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

/**
 * @brief Base class for storage access operations.
 */
class StorageAccessOperation {
public:
    StorageAccessOperation()
        : _targets(0)
    {
    }
    virtual ~StorageAccessOperation() { }

    /**
     * @brief Runs a single iteration of the operation.
     * @return POSIX error code
     */
    virtual int iterate() = 0;

    /**
     * @brief Cancels the operation.
     */
    virtual void cancel() = 0;

    /**
     * @brief Returns whether the operation is still running.
     */
    virtual bool running() const = 0;

    /**
     * @brief Sends a telemetry packet to the configured telemetry targets.
     */
    bool sendTelemetry(frames::frame_type_t type, const uint8_t* payload, uint8_t length)
    {
        return teller::telem::sendTo(_targets, type, payload, length);
    }

    /**
     * @brief Sets the telemetry targets that the operation should be reporting its progress to.
     */
    void setTelemetryTargets(uint8_t targets)
    {
        _targets = targets;
    }

protected:
    uint8_t _targets;
};

/**
 * @brief State object of a raw image read operation from a storage area.
 */
class RawStorageReadOperation : public StorageAccessOperation {
public:
    int start(storage_area_t area, uint64_t address, uint16_t length, uint8_t seq_no);
    virtual bool running() const override;
    virtual int iterate() override;
    virtual void cancel() override;

private:
    storage_area_t _area;
    uint64_t _address;
    uint16_t _bytesLeft;
    frames::binary_data_t _binaryData;
    uint8_t _buf[MAX_PAYLOAD_LENGTH];
};

/**
 * @brief State object of an operation that reads the list of files from a directory.
 */
class DirectoryListingOperation : public StorageAccessOperation {
public:
    int start(storage_area_t area, const char* name, uint16_t start, uint16_t count, uint8_t seq_no);
    virtual bool running() const override;
    virtual int iterate() override;
    virtual void cancel() override;

private:
    storage_area_t _area;
    uint16_t _start;
    uint16_t _count;
    frames::directory_entry_data_t _entryData;
    uint8_t _buf[MAX_PAYLOAD_LENGTH];
    std::unique_ptr<teller::utils::SmartDirHandle> _dir;
};

/**
 * @brief Object containing the state of storage access operations.
 *
 * There is only one instance of this class, and at any point in time it takes
 * care of _one_ reading operation from the storage, from the following list:
 *
 * - listing the contents of a directory
 * - reading the raw contents of the image on the storage device
 * - reading the contents of a single file on the storage device
 *
 * It is the responsibility of the storage module to call the `iterate()` method
 * of the single instance of this class to keep the operation running. New
 * operations can be started with the appropriate methods; starting a new
 * operation while another one is running will return false. An already-running
 * operation can be cancelled explicitly with the `cancel()` method.
 *
 * Internally, the `iterate()` method of this class delegates the actual work
 * to a wrapped instance of `RawStorageReadOperation`, `FileReaderState` or
 * `DirectoryListingState`.
 */
class StorageOperationManager {
public:
    StorageOperationManager()
        : _operation(nullptr)
        , _seq_no(0)
        , _targets(0)
    {
    }
    ~StorageOperationManager()
    {
        cancel();
    }

    void cancel()
    {
        _setOperation(nullptr);
    }

    int iterate()
    {
        int error;

        if (!_operation) {
            // Nothing to do at the moment, but we need to make this function
            // an opportunity to switch tasks so we sleep
            teller::hal::system::delayMsec(50);
        } else {
            error = _operation->iterate();
            if (error) {
                // Error occurred, cancel the operation
                _setOperation(nullptr);
                return error;
            }

            if (!_operation->running()) {
                // Operation finished
                _setOperation(nullptr);
            }
        }

        return 0;
    }

    bool running() const
    {
        return _operation ? _operation->running() : false;
    }

    void setSequenceNumber(uint8_t seq_no)
    {
        _seq_no = seq_no;
    }

    void setTelemetryTargets(uint8_t targets)
    {
        _targets = targets;
    }

    int startDirectoryListing(storage_area_t area, const char* name, uint16_t start, uint16_t count)
    {
        int error;
        std::unique_ptr<DirectoryListingOperation> op = std::make_unique<DirectoryListingOperation>();

        if (!op) {
            return ENOMEM;
        }

        error = op->start(area, name, start, count, _seq_no);
        if (error) {
            return error;
        }

        _setOperation(op.release());

        return 0;
    }

    int startReadingStorage(storage_area_t area, uint64_t address, uint16_t length)
    {
        int error;
        std::unique_ptr<RawStorageReadOperation> op = std::make_unique<RawStorageReadOperation>();

        if (!op) {
            return ENOMEM;
        }

        error = op->start(area, address, length, _seq_no);
        if (error) {
            return error;
        }

        _setOperation(op.release());

        return 0;
    }

    void _setOperation(StorageAccessOperation* op)
    {
        if (_operation) {
            _operation->cancel();
        }

        if (op) {
            op->setTelemetryTargets(_targets);
        }

        _operation.reset(op);
    }

private:
    /** The current storage operation */
    std::unique_ptr<StorageAccessOperation> _operation;

    /** The sequence number of the message that initiated the current request */
    uint8_t _seq_no;

    /** The telemetry targets of the current or next operation */
    uint8_t _targets;
};

static StorageOperationManager storageAccess;

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

int startDirectoryListing(
    teller::telem::storage_area_t area, const char* name, uint16_t start, uint16_t count,
    uint8_t targets, uint8_t seq_no)
{
    int error;

    if (!storageAccess.running()) {
        storageAccess.setTelemetryTargets(targets);
        storageAccess.setSequenceNumber(seq_no);
        error = storageAccess.startDirectoryListing(area, name, start, count);
        if (error) {
            return error;
        }
    } else {
        return EBUSY;
    }

    return 0;
}

int startReadingStorage(
    teller::telem::storage_area_t area, uint64_t address, uint16_t length,
    uint8_t targets, uint8_t seq_no)
{
    int error;

    if (!storageAccess.running()) {
        storageAccess.setTelemetryTargets(targets);
        storageAccess.setSequenceNumber(seq_no);
        error = storageAccess.startReadingStorage(area, address, length);
        if (error) {
            return error;
        }
    } else {
        return EBUSY;
    }

    return 0;
}

void runStorageReader()
{
    while (true) {
        if (storageAccess.iterate()) {
            /* Error during the operation, cancel it */
            storageAccess.cancel();
        };
    }
}

}

/* ************************************************************************* */

int RawStorageReadOperation::start(storage_area_t area, uint64_t address, uint16_t length, uint8_t seq_no)
{
    if (running()) {
        return EBUSY;
    }

    if (length == 0) {
        return EINVAL;
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

    return 0;
}

bool RawStorageReadOperation::running() const
{
    return _bytesLeft > 0;
}

int RawStorageReadOperation::iterate()
{
    const uint8_t limit = MAX_BINARY_DATA_FRAGMENT_LENGTH;

    auto _filesystem = fs.getState(_area, /* ensureMounted = */ false);
    if (!_filesystem) {
        return EIO;
    }

    while (running()) {
        _binaryData.data_length = _bytesLeft > limit ? limit : _bytesLeft;
        if (!_filesystem->readRawData(_binaryData.data, _address, _binaryData.data_length)) {
            break;
        }

        uint8_t length = frames::encodeBinaryDataFrame(&_binaryData, _buf);
        if (!sendTelemetry(frames::BINARY_DATA, _buf, length)) {
            /* Not an error, buffer is full, we'll try later */
            return 0;
        }

        _address += _binaryData.data_length;
        _bytesLeft -= _binaryData.data_length;
        _binaryData.fragment_index++;
    }

    return 0;
}

void RawStorageReadOperation::cancel()
{
    _bytesLeft = 0;
}

/* ************************************************************************* */

int DirectoryListingOperation::start(storage_area_t area, const char* name, uint16_t start, uint16_t count, uint8_t seq_no)
{
    if (running()) {
        return EBUSY;
    }

    if (count == 0) {
        return EINVAL;
    }

    auto fsState = fs.getState(area, /* ensureMounted = */ false);
    if (!fsState) {
        return EIO;
    }

    auto fs = fsState->getFilesystemIfMounted();
    if (!fs) {
        return EIO;
    }

    auto dir = fs->dir_open(name);
    if (std::holds_alternative<littlefs::Error>(dir)) {
        return teller::storage::convertLittleFSErrorCode(std::get<littlefs::Error>(dir));
    }

    _dir = std::make_unique<teller::utils::SmartDirHandle>(fs, std::get<littlefs::DirHandle>(dir));
    if (!_dir) {
        return ENOMEM;
    }

    auto maybe_error = _dir->seek(start);
    if (maybe_error.has_value()) {
        if (_dir->hasMoreEntries()) {
            return teller::storage::convertLittleFSErrorCode(*maybe_error);
        } else {
            /* Start index was too large but we can still start the listing;
             * this will be handled gracefully in iterate() */
        }
    }

    _area = area;
    _start = start;
    _count = count;

    memset(&_entryData, 0, sizeof(_entryData));

    _entryData.frame_type = frames::DIRECTORY_LISTING;
    _entryData.seq_no = seq_no;
    _entryData.entry_index = 0;
    _entryData.max_entry_index = _count - 1; /* TODO */

    return 0;
}

bool DirectoryListingOperation::running() const
{
    return _count > 0 && _dir && _dir->hasMoreEntries();
}

int DirectoryListingOperation::iterate()
{
    while (running()) {
        std::string name;
        littlefs::Type type;
        size_t size;

        auto maybe_error = _dir->read(name, type, size);
        if (maybe_error) {
            /* ENOENT is returned when we reach the end but we do not want to
             * treat this as an error */
            if (_dir->hasMoreEntries()) {
                return teller::storage::convertLittleFSErrorCode(*maybe_error);
            } else {
                /* This is the last entry, send a telemetry message with an
                 * empty filename */
                _entryData.name[0] = 0;
                _entryData.max_entry_index = _entryData.entry_index;
            }
        } else {
            snprintf(
                _entryData.name, MAX_DIRECTORY_ENTRY_NAME_LENGTH,
                "%s%s", name.c_str(), type == littlefs::Type::DIR ? "/" : "");
        }

        uint8_t length = frames::encodeDirectoryEntryFrame(&_entryData, _buf);
        if (!sendTelemetry(frames::DIRECTORY_ENTRY, _buf, length)) {
            /* Not an error, buffer is full, we'll try later */
            return 0;
        }

        if (_dir->hasMoreEntries()) {
            _entryData.entry_index++;
            _count--;
        } else {
            cancel();
        }
    }

    return 0;
}

void DirectoryListingOperation::cancel()
{
    _dir.reset();
    _count = 0;
}
