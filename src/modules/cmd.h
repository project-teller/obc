#pragma once

#include "core/telem/generic.h"
#include "hal/queue.hpp"
#include "hal/uart.h"

namespace teller::cmd {

/**
 * Initializes the data structures required by the command handler module.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * Destroys the data structures required by the command handler module.
 */
void destroy(void);

/**
 * @brief Feeds a new incoming command to be processed by the command handler module.
 *
 * Blocks when the queue of the command handler is full until a new slot becomes
 * available in the command handler.
 *
 * @param index     the index of the UART that the command was received from
 * @param envelope  the envelope of the incoming command
 * @param payload   the payload of the incoming command
 * @param length    lengrh of the payload
 */
void feed(
    teller::hal::uart::uart_t index,
    const teller::telem::envelope_t& envelope,
    const std::uint8_t* payload, std::uint8_t length);

/**
 * @brief Feeds a new incoming command to be processed by the command handler module.
 *
 * Drops the inbound message immediately if the command handler is full.
 *
 * @param index     the index of the UART that the command was received from
 * @param envelope  the envelope of the incoming command
 * @param payload   the payload of the incoming command
 * @param length    lengrh of the payload
 * @return whether the data was sent successfully to the command handler
 */
bool feedNonblocking(
    teller::hal::uart::uart_t index,
    const teller::telem::envelope_t& envelope,
    const std::uint8_t* payload, std::uint8_t length);

/**
 * @brief Returns a pointer to the inbound queue of the command handler module.
 */
teller::hal::BlockingQueueBase* getQueue(void);

/**
 * @brief Processes the next message waiting in the command handler module.
 *
 * Blocks indefinitely if there are no messages to process.
 *
 * @return whether a message was processed successfully
 */
bool processNext(void);

/**
 * @brief Returns the number of messages waiting in the command handler module.
 */
size_t waiting(void);

}
