#include "core/utils/smart_dir_handle.h"

using namespace littlefs;
using namespace teller::utils;

SmartDirHandle::SmartDirHandle(Filesystem* fs, DirHandle dd)
    : _fs(fs)
    , _dd(dd)
    , _inited(false)
    , _closed(false)
    , _ended(false)
    , _index(0)
{
    assert(fs != nullptr);
}

SmartDirHandle::~SmartDirHandle()
{
    close();
}

SmartDirHandle::SmartDirHandle(SmartDirHandle&& that)
{
    _fs = that._fs;
    _dd = that._dd;
    _inited = that._inited;
    _closed = that._closed;
    _ended = that._ended;
    _index = that._index;
}

std::optional<Error> SmartDirHandle::close()
{
    if (!_closed) {
        if (_inited) {
            auto result = _fs->dir_close(_dd);
            if (result) {
                return result;
            }
        }

        _closed = true;
    }

    return std::nullopt;
}

std::optional<littlefs::Error> SmartDirHandle::read(
    std::string& name, littlefs::Type& type, size_t& size)
{
    auto result = _fs->dir_read(_dd, name, type);
    if (std::holds_alternative<littlefs::Error>(result)) {
        auto error = std::get<littlefs::Error>(result);
        if (error == littlefs::Error::NOENT) {
            _ended = true;
        }
        return error;
    }

    size = std::get<size_t>(result);

    return std::nullopt;
}

std::optional<Error> SmartDirHandle::rewind()
{
    auto result = _fs->dir_rewind(_dd);
    if (result.has_value()) {
        return result;
    }

    _index = 0;
    _ended = false;

    return std::nullopt;
}

std::optional<Error> SmartDirHandle::seek(size_t index)
{
    if (index > _index) {
        auto maybe_error = rewind();
        if (maybe_error.has_value()) {
            return maybe_error;
        }
    }

    while (index < _index) {
        std::string name;
        Type type;
        size_t size;

        auto maybe_error = read(name, type, size);
        if (maybe_error.has_value()) {
            return maybe_error;
        }

        _index++;
    }

    return std::nullopt;
}
