#pragma once

#include "core/telem/generic.h"

namespace teller::telem::frames {

typedef enum {
    STORAGE_COMMAND_NOP,
    STORAGE_COMMAND_MOUNT,
    STORAGE_COMMAND_UNMOUNT,
    STORAGE_COMMAND_ERASE,
    STORAGE_COMMAND_READ,
    NUM_STORAGE_COMMANDS,
} storage_command_t;

/**
 * @brief Structure containing all the data required to construct a request
 * to erase, mount or unmount a storage area.
 */
typedef struct {
    /** Index of the storage area to erase */
    storage_area_t area;

    /** The command to execute */
    storage_command_t command;

    /** The offset for read requests */
    uint32_t offset;

    /** The length for read requests */
    uint16_t length;
} storage_command_data_t;

/**
 * @brief Encodes a storage area related command frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeStorageCommandFrame(const storage_command_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a storage area related command from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeStorageCommandFrame(const std::uint8_t* encoded, storage_command_data_t* decoded);

}
