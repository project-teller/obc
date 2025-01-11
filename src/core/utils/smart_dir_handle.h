#pragma once

#include "littlefs-cpp.h"

namespace teller::utils {

class SmartDirHandle {
public:
    explicit SmartDirHandle(littlefs::Filesystem* fs, littlefs::DirHandle dd);

    SmartDirHandle(SmartDirHandle&& that);
    ~SmartDirHandle();

    SmartDirHandle(const SmartDirHandle&) = delete;
    SmartDirHandle& operator=(const SmartDirHandle&) = delete;

    /**
     * @brief Closes the directory handle. No-op if it is already closed.
     */
    std::optional<littlefs::Error> close();

    /**
     * @brief Returns whether there are more entries to read from the handle.
     */
    bool hasMoreEntries() const { return !_ended; }

    /**
     * @brief Reads the next entry from the directory handle.
     *
     * @param name  the name of the next entry will be returned here
     * @param type  the type of the next entry will be returned here
     * @param size  the size of the next entry will be returned here
     *
     * @return littlefs::ENOENT if there are no more entries, or a LittleFS error code
     */
    std::optional<littlefs::Error> read(std::string& name, littlefs::Type& type, size_t& size);

    /**
     * @brief Rewinds the directory handle.
     */
    std::optional<littlefs::Error> rewind();

    /**
     * @brief Seeks the directory handle to the entry with the given index.
     *
     * This function may rewind the handle and start iterating again if we
     * need to seek backwards.
     */
    std::optional<littlefs::Error> seek(size_t index);

private:
    littlefs::Filesystem* _fs;
    littlefs::DirHandle _dd;
    bool _inited;
    bool _closed;
    bool _ended;
    size_t _index;
};

}
