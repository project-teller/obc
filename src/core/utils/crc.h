#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc_ccitt(uint16_t crc, const uint8_t* buf, size_t len);
uint8_t crc7_sd(uint8_t crc, const uint8_t* buf, size_t len);

#ifdef __cplusplus
}
#endif
