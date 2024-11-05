#pragma once

#include "core/math/vector.hpp"
#include "core/telem/generic.h"

namespace teller::telem::frames {

/**
 * @brief Structure containing all the data required to construct an ADC frame.
 *
 * This structure is not the same as the wire encoding of the ADC frame.
 * It contains the raw values of the fields, in SI units where applicable
 * (amps for current, volts for voltage).
 */
typedef struct {
    uint32_t timestampInMsec;
    union {
        float byIndex[13];
        struct {
            float currentSUCLCL1;
            float currentSUCLCL2;
            float currentSUCLCL3;
            float currentCAM;
            float currentSCM;
            float currentGMM;
            float voltage60V1;
            float voltage60V2;
            float voltage60V3;
            float voltage28V;
            float voltage12V;
            float voltage5V;
            float voltage3V3;
        } byName;
    } measurements;
} adc_data_t;

/**
 * @brief Encodes an ADC frame into its wire representation.
 *
 * @param data  the data to encode in the telemetry frame
 * @param encoded  the encoded representation of the telemetry frame
 * @return the length of the encoded representation of the telemetry frame
 */
uint8_t encodeADCFrame(const adc_data_t* data, std::uint8_t* encoded);

/**
 * @brief Decodes an ADC frame from its wire representation.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param decoded  the decoded representation of the telemetry frame
 */
void decodeADCFrame(const std::uint8_t* encoded, adc_data_t* decoded);

/**
 * @brief Validates the wire representation of an ADC frame without decoding it.
 *
 * @param encoded  the encoded representation of the telemetry frame
 * @param length   the length of the encoded representation of the frame
 * @return whether the frame can be considered valid and is safe to be decoded
 */
bool validateEncodedADCFrame(const uint8_t* encoded, size_t length);

}
