#pragma once

#include <cstdint>

namespace teller::hal::usb {

/**
 * @brief Initialization function for the USB subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the USB subsystem.
 *
 * This function is called from tests to reset the USB subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Prepares the USB subsystem.
 *
 * This is where the real initialization will happen, after we have started the
 * FreeRTOS scheduler.
 */
[[nodiscard]] bool setup(void);

/**
 * @brief Returns whether the USB cable is connected.
 */
bool isConnected(void);

/**
 * @brief Reads at most a given number of raw bytes from the USB port.
 *
 * The function will block and yield to other tasks until at least one byte is
 * available to read from the USB port.
 *
 * @param data the buffer to read into
 * @param size the number of bytes to read
 * @param bytes_read the number of bytes that were read. May be less than the
 *        size when EOF is reached or when an IO error occurred.
 * @return whether the read was successful
 */
bool read(std::uint8_t* data, std::uint16_t size, std::uint16_t* bytes_read);

/**
 * @brief Writes a buffer containing raw bytes to the USB port.
 *
 * The function will block and yield to other tasks until all the bytes have
 * been written to the USB port.
 *
 * @param data the buffer containing the bytes to write
 * @param size the number of bytes to write
 * @return whether the write was successful
 */
bool write(std::uint8_t* data, std::uint16_t size);

}
