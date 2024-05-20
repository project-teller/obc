#pragma once

#include "littlefs-cpp.h"

namespace teller::utils {

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

}
