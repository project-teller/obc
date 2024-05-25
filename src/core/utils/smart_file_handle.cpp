#include "core/utils/smart_file_handle.h"

using namespace littlefs;
using namespace teller::utils;

#define IS_ERROR_VARIANT(result_) (std::holds_alternative<Error>(result_))
#define THROW_IF_FAILED_VARIANT(result_)        \
    {                                           \
        auto maybe_err__ = result_;             \
        if (IS_ERROR_VARIANT(maybe_err__)) {    \
            throw std::get<Error>(maybe_err__); \
        }                                       \
    }
#define THROW_IF_FAILED(result_)       \
    {                                  \
        auto maybe_err__ = result_;    \
        if (maybe_err__.has_value()) { \
            throw *maybe_err__;        \
        }                              \
    }

SmartFileHandle::SmartFileHandle(Filesystem* fs, int fd)
    : _fs(fs)
    , _fd(fd)
    , _closed(false)
{
    assert(fs != nullptr);
}

SmartFileHandle::SmartFileHandle(Filesystem* fs, FileHandle fd)
    : _fs(fs)
    , _fd(fd)
    , _closed(false)
{
    assert(fs != nullptr);
}

SmartFileHandle::SmartFileHandle(Filesystem* fs, std::variant<Error, FileHandle> fd)
    : _fs(fs)
    , _fd()
    , _inited(false)
    , _closed(false)
{
    assert(fs != nullptr);
    THROW_IF_FAILED_VARIANT(fd);

    _inited = true;
    _fd = std::get<FileHandle>(fd);
}

SmartFileHandle::~SmartFileHandle()
{
    close();
}

SmartFileHandle::SmartFileHandle(SmartFileHandle&& that)
{
    _fs = that._fs;
    _fd = that._fd;
    _inited = that._inited;
    _closed = that._closed;
}

std::optional<Error> SmartFileHandle::close()
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

std::optional<Error> SmartFileHandle::sync()
{
    return _fs->sync(_fd);
}

size_t SmartFileHandle::write(void* write_buf, size_t bytes_to_write)
{
    auto result = _fs->write(_fd, write_buf, bytes_to_write);
    THROW_IF_FAILED_VARIANT(result);
    return std::get<size_t>(result);
}
