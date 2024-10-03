#include "core/utils/smart_file_handle.h"

using namespace littlefs;
using namespace teller::utils;

#define IS_ERROR_VARIANT(result_) (std::holds_alternative<Error>(result_))

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

std::variant<Error, size_t> SmartFileHandle::read(void* read_buf, size_t bytes_to_read)
{
    return _fs->read(_fd, read_buf, bytes_to_read);
}

std::optional<Error> SmartFileHandle::sync()
{
    return _fs->sync(_fd);
}

std::variant<Error, size_t> SmartFileHandle::write(void* write_buf, size_t bytes_to_write)
{
    return _fs->write(_fd, write_buf, bytes_to_write);
}
