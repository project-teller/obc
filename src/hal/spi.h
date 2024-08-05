#pragma once

#include <cstdint>

namespace teller::hal::spi {

typedef struct {
    std::uint8_t bus;
    std::uint8_t device;
} address_t;

typedef struct {
    /**< Transfer buffer of the transaction; null means that this is a guard element */
    std::uint8_t* tx_buf;

    /**< Receive buffer of the transaction; null means same as transfer buffer */
    std::uint8_t* rx_buf;

    /**< Number of bytes to transfer */
    std::uint16_t size;
} transfer_t;

typedef enum {
    /** Do not pull CS to low during transfer. Used when initializing an SD card */
    NO_CHIP_SELECT = 1
} transfer_flags_t;

/** Dummy address that is used for devices that are not configured or present */
extern const address_t NO_ADDRESS;

/** Guard element that denotes that there are no more transfers */
extern const transfer_t NO_MORE_TRANSFERS;

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
 * @brief Selects the SPI device with the given address.
 *
 * Normally you should not need to use this function as transfer() takes care of
 * this for you -- unless you use the NO_CHIP_SELECT flag.
 *
 * @return whether the operation was successful
 */
bool select(address_t address, bool value = true);

/**
 * @brief Runs a simultaneous transmit-receive cycle on an SPI device.
 *
 * The function will block and yield to other tasks until the entire buffer was
 * processed on the SPI bus.
 *
 * @param address the SPI address to use
 * @param buf the buffer to use
 * @param size the common length of the buffers
 * @param flags flags to modify the behaviour of the transfer in special cases
 * @return whether the operation was successful
 */
bool transfer(
    address_t address, std::uint8_t* buf, std::uint16_t size, std::uint8_t flags = 0);

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
 * @param flags flags to modify the behaviour of the transfer in special cases
 * @return whether the operation was successful
 */
bool transfer(
    address_t address, std::uint8_t* txBuf, std::uint8_t* rxBuf,
    std::uint16_t size, std::uint8_t flags = 0);

/**
 * @brief Runs a possibly multi-transaction transmit-receive cycle on an SPI device.
 *
 * In a multi-transaction transmit-receive cycle, the chip select pin of the
 * SPI device is held low for the entire transaction, but multiple individual
 * SPI transfers are performed with separate transmit and receive buffers. The
 * buffers to use in individual transfers in the transaction are stored in
 * a \ref transfer_t structure. The maximum number of transfers must be
 * specified in advance. The transaction stops when a transfer in the array of
 * transfers has a null pointer in its transmit buffer, or when the maximum
 * number of transfers has been performed. You can use the \c NO_MORE_TRANSFERS
 * constant as a guard element.
 *
 * @param address   the SPI address to use
 * @param transfers the transfers to perform in the transaction
 * @param count     the maximum number of transfers to perform; zero means an
 *        infinite amount of transfers until the \c NO_MORE_TRANSFERS guard
 *        element is reached
 * @param flags flags to modify the behaviour of the transfer in special cases
 */
bool transfer(
    address_t address, const transfer_t* transfers, std::uint16_t count,
    std::uint8_t flags = 0);

/**
 * @brief Unselects the SPI device with the given address.
 *
 * Normally you should not need to use this function as transfer() takes care of
 * this for you -- unless you use the NO_CHIP_SELECT flag.
 *
 * @return whether the operation was successful
 */
bool unselect(address_t address);

int getLastErrorCode(void);

}
