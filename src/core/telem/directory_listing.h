#pragma once

#include "core/telem/generic.h"

namespace teller::telem {

const int MAX_DIRECTORY_LISTING_NAME_LENGTH = MAX_PAYLOAD_LENGTH - 5;

};

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a request
 * to read the list of files in a directory on a storage area.
 */
typedef struct {
    /** Index of the storage area */
    storage_area_t area;

    /** The start index of the entries to return */
    uint16_t start;

    /** The maximum number of entries to return */
    uint16_t count;

    /**
     * The name of the directory to list, null-terminated. Note that the maximum allowed
     * message length is therefore \c MAX_PAYLOAD_LENGTH - 5 because five bytes
     * are consumed by the storage area, the start index and the number of
     * entries. The buffer has a length of \c MAX_PAYLOAD_LENGTH - 4 to allow
     * for the trailing zero in the string, but the trailing zero is not transmitted.
     */
    char name[MAX_DIRECTORY_LISTING_NAME_LENGTH + 1];
} directory_listing_request_data_t;

/**
 * @brief Encodes a directory listing request frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeDirectoryListingRequestFrame(
    const directory_listing_request_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a directory listing request frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeDirectoryListingRequestFrame(
    const std::uint8_t* encoded, size_t length, directory_listing_request_data_t* decoded);

/**
 * @brief Validates the wire representation of a directory listing request frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedDirectoryListingRequestFrame(const uint8_t* encoded, size_t length);

}
