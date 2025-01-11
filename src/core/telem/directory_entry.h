#pragma once

#include "core/telem/generic.h"

namespace teller::telem {

const int MAX_DIRECTORY_ENTRY_NAME_LENGTH = MAX_PAYLOAD_LENGTH - 5;

};

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a directory
 * entry frame.
 */
typedef struct {
    /** Type of the frame that the directory entry frame responds to */
    frame_type_t frame_type;

    /** Sequence number of the frame that the directory entry frame responds to */
    uint8_t seq_no;

    /** Maximum entry index in the batch being sent */
    uint8_t max_entry_index;

    /** Current entry index in the batch being sent */
    uint8_t entry_index;

    /** File or directory name to send in the frame */
    char name[MAX_DIRECTORY_ENTRY_NAME_LENGTH + 1];
} directory_entry_data_t;

/**
 * @brief Encodes a directory entry frame into its wire representation.
 *
 * @param data  the data to encode in the directory entry frame
 * @param encoded  the encoded representation of the directory entry frame
 * @return the length of the encoded representation of the directory entry frame
 */
uint8_t encodeDirectoryEntryFrame(const directory_entry_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a directory entry frame from its wire representation.
 *
 * @param encoded  the encoded representation of the directory entry frame
 * @param decoded  the decoded representation of the directory entry frame
 */
void decodeDirectoryEntryFrame(const std::uint8_t* encoded, size_t length, directory_entry_data_t* decoded);

/**
 * @brief Validates the wire representation of a directory entry frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedDirectoryEntryFrame(const uint8_t* encoded, size_t length);

}
