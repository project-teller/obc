#pragma once

#include "core/telem/generic.h"

namespace teller::telem {

const int MAX_FILE_DOWNLOAD_PATH_LENGTH = MAX_PAYLOAD_LENGTH - 7;

};

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct a request
 * to read the list of files in a directory on a storage area.
 */
typedef struct {
    /** Index of the storage area */
    storage_area_t area;

    /** The offset to start reading from the file */
    uint32_t start;

    /** The maximum number of bytes to return */
    uint16_t length;

    /**
     * The full path of the file to download, null-terminated. Note that the maximum allowed
     * message length is therefore \c MAX_PAYLOAD_LENGTH - 7 because seven bytes
     * are consumed by the reset of the packet. The buffer has a length of
     * \c MAX_PAYLOAD_LENGTH - 6 to allow for the trailing zero in the string,
     * but the trailing zero is not transmitted.
     */
    char path[MAX_FILE_DOWNLOAD_PATH_LENGTH + 1];
} file_download_request_data_t;

/**
 * @brief Encodes a file download request frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeFileDownloadRequestFrame(
    const file_download_request_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes a file download request frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeFileDownloadRequestFrame(
    const std::uint8_t* encoded, size_t length, file_download_request_data_t* decoded);

/**
 * @brief Validates the wire representation of a file download request frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedFileDownloadRequestFrame(const uint8_t* encoded, size_t length);

}
