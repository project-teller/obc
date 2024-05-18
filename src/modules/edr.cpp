#include <cassert>
#include <cstdarg>
#include <optional>

#include <sdlog/sdlog.h>

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
/* Helper macros                                                             */
/* ************************************************************************* */

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
     * @return the number of bytes that were read
     */
    size_t read(void* read_buf, size_t bytes_to_read);

    /**
     * @brief Syncs the file handle with the on-disk representation.
     */
    std::optional<littlefs::Error> sync();

    /**
     * @brief Writes the given number of bytes from the file handle.
     *
     * @param buf   the buffer to write
     * @param bytes_to_write  the size of the buffer
     * @return the number of bytes that were written
     */
    size_t write(void* conwrite_buf, size_t bytes_to_write);

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
    THROW_IF_FAILED_VARIANT(fd);

    _inited = true;
    _fd = std::get<littlefs::FileHandle>(fd);
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

size_t SmartFileHandle::read(void* read_buf, size_t bytes_to_read)
{
    auto result = _fs->read(_fd, read_buf, bytes_to_read);
    THROW_IF_FAILED_VARIANT(result);
    return std::get<size_t>(result);
}

std::optional<littlefs::Error> SmartFileHandle::sync()
{
    return _fs->sync(_fd);
}

size_t SmartFileHandle::write(void* write_buf, size_t bytes_to_write)
{
    auto result = _fs->write(_fd, write_buf, bytes_to_write);
    THROW_IF_FAILED_VARIANT(result);
    return std::get<size_t>(result);
}

/* ************************************************************************* */
/* LogWriter implementation                                                  */
/* ************************************************************************* */

static sdlog_error_t log_writer_write(
    sdlog_ostream_t* self, const uint8_t* data, size_t length, size_t* bytes_written);
static sdlog_error_t log_writer_flush(sdlog_ostream_t* self);

static const sdlog_ostream_spec_t log_writer_spec = {
    .write = log_writer_write,
    .flush = log_writer_flush,
};

class LogWriter {
public:
    explicit LogWriter(SmartFileHandle& handle);
    ~LogWriter();

    LogWriter(const LogWriter&) = delete;
    LogWriter& operator=(const LogWriter&) = delete;

    void flush();
    void write(const sdlog_message_format_t* fmt, ...);

    void _flush_raw();
    size_t _write_raw(const uint8_t* data, size_t length);

private:
    SmartFileHandle& _handle;
    sdlog_ostream_t _stream;
    sdlog_writer_t _writer;
    int _init_state;
};

LogWriter::LogWriter(SmartFileHandle& handle)
    : _handle(handle)
    , _init_state(0)
{
    if (sdlog_ostream_init(&_stream, &log_writer_spec, this)) {
        throw std::bad_alloc();
    }
    _init_state++;

    if (sdlog_writer_init(&_writer, &_stream)) {
        throw std::bad_alloc();
    }
    _init_state++;
}

LogWriter::~LogWriter()
{
    if (_init_state > 1) {
        sdlog_writer_destroy(&_writer);
        _init_state--;
    }
    if (_init_state > 0) {
        sdlog_ostream_destroy(&_stream);
        _init_state--;
    }
}

void LogWriter::flush()
{
    if (sdlog_writer_flush(&_writer) != SDLOG_SUCCESS) {
        throw littlefs::Error::IO;
    }
}

void LogWriter::write(const sdlog_message_format_t* fmt, ...)
{
    sdlog_error_t err;
    va_list args;

    va_start(args, fmt);
    err = sdlog_writer_write_va(&_writer, fmt, args);
    va_end(args);

    if (err != SDLOG_SUCCESS) {
        throw littlefs::Error::IO;
    }
}

void LogWriter::_flush_raw()
{
    _handle.sync();
}

size_t LogWriter::_write_raw(const uint8_t* data, size_t length)
{
    return _handle.write(const_cast<uint8_t*>(data), length);
}

static sdlog_error_t log_writer_write(
    sdlog_ostream_t* self, const uint8_t* data, size_t length, size_t* bytes_written)
{
    LogWriter* writer = reinterpret_cast<LogWriter*>(self->context);
    *bytes_written = writer->_write_raw(data, length);
    return SDLOG_SUCCESS;
}

static sdlog_error_t log_writer_flush(sdlog_ostream_t* self)
{
    LogWriter* writer = reinterpret_cast<LogWriter*>(self->context);
    writer->_flush_raw();
    return SDLOG_SUCCESS;
}

/* ************************************************************************* */
/* ExperimentDataRecorder implementation                                     */
/* ************************************************************************* */

const std::string LASTLOG_FILE("LASTLOG.TXT");

void ExperimentDataRecorder::run()
{
    ssize_t logIndex = getLastLogIndex();
    char fname[32];

    logIndex = logIndex < 0 ? 0 : (logIndex + 1);
    updateLastLogIndex(logIndex);

    snprintf(fname, sizeof(fname), "%08ld.BIN", static_cast<long int>(logIndex));
    SmartFileHandle fd(this->_fs, this->_fs->open(fname, littlefs::OpenFlag::WRONLY | littlefs::OpenFlag::CREAT | littlefs::OpenFlag::TRUNC));
    LogWriter writer(fd);

    /* TODO: write log entries into the file */

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
    fd.read(buf, sizeof(buf));
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
    int num_printed = snprintf(buf, sizeof(buf), "%lu", static_cast<long unsigned int>(index));
    if (fd.write(buf, num_printed) != num_printed) {
        throw littlefs::Error::IO;
    }
}
}
