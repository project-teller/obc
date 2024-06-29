#pragma once

#include <cstdint>
#include <cstdlib>

namespace teller::hal::uart {

typedef enum {
    TELEMETRY,
    DEBUG,
    SINK,
    NUM_UARTS,
} uart_t;

/**
 * @brief Initialization function for the UART subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the UART subsystem.
 *
 * This function is called from tests to reset the UART subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Returns whether the UART with the given index is connected.
 *
 * Most UARTs are defined to be connected all the time. The only exception is
 * the external USB debug port, where we can detect whether a USB cable is
 * connected or not, and return a value accordingly.
 */
bool isConnected(uart_t index);

/**
 * @brief Reads exactly a given number of raw bytes from a UART.
 *
 * The function will block and yield to other tasks until the given number of
 * bytes were read from the UART.
 *
 * @param index the index of the UART to read from
 * @param data the buffer to read into
 * @param size the number of bytes to read
 * @param bytes_read the number of bytes that were read. May be less than the
 *        size when EOF is reached or when an IO error occurred.
 * @return whether the read was successful
 */
bool read(uart_t index, std::uint8_t* data, std::uint16_t size, uint16_t* bytes_read);

/**
 * @brief Waits until the given UART becomes connected.
 */
void waitUntilConnected(uart_t index);

/**
 * @brief Waits until the given UART becomes disconnected.
 */
void waitUntilDisconnected(uart_t index);

/**
 * @brief Writes a buffer containing raw bytes to a UART.
 *
 * The function will block and yield to other tasks until all the bytes have
 * been written to the UART.
 *
 * @param index the index of the UART to write to
 * @param data the buffer containing the bytes to write
 * @param size the number of bytes to write
 * @return whether the write was successful
 */
bool write(uart_t index, std::uint8_t* data, std::uint16_t size);

/**
 * @brief Writes a null-terminated string to the UART.
 *
 * The function will block and yield to other tasks until the entire string has
 * been written to the UART.
 *
 * @param index the index of the UART to write to
 * @param data the string to write
 * @return whether the write was successful
 */
bool write(uart_t index, const char* data);

}
