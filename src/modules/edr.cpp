#include <cassert>
#include <optional>

#include "hal/storage.h"
#include "hal/system.h"

#include "modules/log.h"
#include "modules/storage.h"

#include "modules/edr.h"

namespace teller::edr {

bool init()
{
    return true;
}

void destroy()
{
}

/* ************************************************************************* */
/* SmartFileHandle implementation                                            */
/* ************************************************************************* */

class SmartFileHandle {
public:
    explicit SmartFileHandle(littlefs::Filesystem* fs, int fd);
    explicit SmartFileHandle(littlefs::Filesystem* fs, littlefs::FileHandle fd);
    explicit SmartFileHandle(littlefs::Filesystem* fs, std::variant<littlefs::Error, littlefs::FileHandle> fd);

    ~SmartFileHandle();

    SmartFileHandle(const SmartFileHandle&) = delete;
    SmartFileHandle& operator=(const SmartFileHandle&) = delete;

    /**
     * @brief Closes the file handle. No-op if it is already closed.
     */
    std::optional<littlefs::Error> close();

    /**
     * @brief Reads at most the given number of bytes from the file handle.
     *
     * @param read_buf   the buffer to read into
     * @param bytes_to_read  the number of bytes to read
     */
    std::optional<littlefs::Error> read(void* read_buf, size_t bytes_to_read);

    /**
     * @brief Writes the given number of bytes from the file handle.
     *
     * @param buf   the buffer to write
     * @param bytes_to_write  the size of the buffer
     */
    std::optional<littlefs::Error> write(void* conwrite_buf, size_t bytes_to_write);

private:
    littlefs::Filesystem* _fs;
    littlefs::FileHandle _fd;
    bool _inited;
    bool _closed;
};

SmartFileHandle::SmartFileHandle(littlefs::Filesystem* fs, int fd)
    : _fs(fs)
    , _fd(fd)
    , _closed(false)
{
    assert(fs != nullptr);
}

SmartFileHandle::SmartFileHandle(littlefs::Filesystem* fs, littlefs::FileHandle fd)
    : _fs(fs)
    , _fd(fd)
    , _closed(false)
{
    assert(fs != nullptr);
}

SmartFileHandle::SmartFileHandle(littlefs::Filesystem* fs, std::variant<littlefs::Error, littlefs::FileHandle> fd)
    : _fs(fs)
    , _fd()
    , _inited(false)
    , _closed(false)
{
    assert(fs != nullptr);

    if (std::holds_alternative<littlefs::Error>(fd)) {
        throw std::get<littlefs::Error>(fd);
    } else {
        _inited = true;
        _fd = std::get<littlefs::FileHandle>(fd);
    }
}

SmartFileHandle::~SmartFileHandle()
{
    close();
}

std::optional<littlefs::Error> SmartFileHandle::close()
{
    if (!_closed) {
        if (_inited) {
            auto result = _fs->close(_fd);
            if (result) {
                return result;
            }
        }

        _closed = true;
    }

    return std::nullopt;
}

std::optional<littlefs::Error> SmartFileHandle::read(void* read_buf, size_t bytes_to_read)
{
    auto result = _fs->read(_fd, read_buf, bytes_to_read);
    if (std::holds_alternative<littlefs::Error>(result)) {
        return std::get<littlefs::Error>(result);
    } else {
        return std::nullopt;
    }
}

std::optional<littlefs::Error> SmartFileHandle::write(void* write_buf, size_t bytes_to_write)
{
    auto result = _fs->write(_fd, write_buf, bytes_to_write);
    if (std::holds_alternative<littlefs::Error>(result)) {
        return std::get<littlefs::Error>(result);
    } else {
        return std::nullopt;
    }
}

/* ************************************************************************* */
/* ExperimentDataRecorder implementation                                     */
/* ************************************************************************* */

const std::string LASTLOG_FILE("LASTLOG.TXT");

#define IS_ERROR_VARIANT(result_) (std::holds_alternative<littlefs::Error>(result_))
#define THROW_IF_FAILED_VARIANT(result_)                  \
    {                                                     \
        auto maybe_err__ = result_;                       \
        if (IS_ERROR_VARIANT(maybe_err__)) {              \
            throw std::get<littlefs::Error>(maybe_err__); \
        }                                                 \
    }
#define THROW_IF_FAILED(result_)       \
    {                                  \
        auto maybe_err__ = result_;    \
        if (maybe_err__.has_value()) { \
            throw *maybe_err__;        \
        }                              \
    }

void ExperimentDataRecorder::run()
{
    ssize_t logIndex = getLastLogIndex();
    logIndex = logIndex < 0 ? 0 : (logIndex + 1);
    updateLastLogIndex(logIndex);

    teller::hal::system::sleepForever();
}

/**
 * @brief Returns the index of the last log file.
 *
 * @return the index of the last log file; zero if no log file was created yet
 */
size_t ExperimentDataRecorder::getLastLogIndex()
{
    /* Find and read LASTLOG.TXT, increase the counter by 1 */
    char buf[32];
    littlefs::Info info;

    /* Check if LASTLOG.TXT exists */
    if (this->_fs->stat(LASTLOG_FILE, &info)) {
        /* File does not exist, return 0 */
        return 0;
    }

    /* Check if LASTLOG.TXT is a regular file */
    if (info.type != LFS_TYPE_REG) {
        throw littlefs::Error::ISDIR;
    }

    /* Open LASTLOG.TXT */
    long int index;
    SmartFileHandle fd(this->_fs, this->_fs->open(LASTLOG_FILE, littlefs::OpenFlag::RDONLY));

    /* Read file contents */
    memset(buf, 0, sizeof(buf));
    THROW_IF_FAILED(fd.read(buf, sizeof(buf)));
    if (sscanf(buf, "%ld", &index) != 1) {
        index = 0;
    }

    return index < 0 ? 0 : index;
}

/**
 * @brief Updates the index of the last log file.
 * @return whether the operation was successful
 */
void ExperimentDataRecorder::updateLastLogIndex(size_t index)
{
    /* Open LASTLOG.TXT */
    SmartFileHandle fd(this->_fs, this->_fs->open(LASTLOG_FILE, littlefs::OpenFlag::WRONLY | littlefs::OpenFlag::CREAT | littlefs::OpenFlag::TRUNC));
    char buf[32];
    int num_printed = snprintf(buf, sizeof(buf), "%lu", index);
    assert(num_printed < sizeof(buf));
    THROW_IF_FAILED(fd.write(buf, num_printed));
}

}
