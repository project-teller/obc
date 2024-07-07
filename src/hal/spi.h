#pragma once

#include <cstdint>

namespace teller::hal::spi {

typedef struct {
    std::uint8_t bus;
    std::uint8_t device;
} address_t;

/**
 * @brief Initialization function for the SPI bus subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destructor function for the SPI bus subsystem.
 *
 * This function is called from tests to reset the LED subsystem to a known
 * base state.
 */
void destroy(void);

/**
 * @brief Runs a simultaneous transmit-receive cycle on an SPI device.
 *
 * The function will block and yield to other tasks until the entire buffer was
 * processed on the SPI bus.
 *
 * @param address the SPI address to use
 * @param buf the buffer to use
 * @param size the common length of the buffers
 * @return whether the operation was successful
 */
bool transfer(address_t address, std::uint8_t* buf, std::uint16_t size);

/**
 * @brief Runs a simultaneous transmit-receive cycle on an SPI device, using separate buffers.
 *
 * The function will block and yield to other tasks until the entire buffer was
 * processed on the SPI bus.
 *
 * @param address the SPI address to use
 * @param txBuf the transmit buffer to use
 * @param rxBuf the receive buffer to use
 * @param size the common length of the buffers
 * @return whether the operation was successful
 */
bool transfer(address_t address, std::uint8_t* txBuf, std::uint8_t* rxBuf, std::uint16_t size);

}
