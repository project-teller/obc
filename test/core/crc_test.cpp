#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/crc.h"

TEST(CRCTest, crc_ccitt)
{
    uint8_t buf[] = "123456789";
    size_t buf_len = std::strlen(reinterpret_cast<char*>(buf));

    EXPECT_EQ(0, crc_ccitt(0, buf, 0));
    EXPECT_EQ(0x2189, crc_ccitt(0, buf, buf_len));
    EXPECT_EQ(0xe026, crc_ccitt(0x2189, buf, buf_len));
}

TEST(CRCTest, crc7_sd)
{
    uint8_t buf[5] = { 0x40, 0x00, 0x00, 0x00, 0x00 }; /* SD card CMD0(0) */
    uint8_t buf2[5] = { 0x48, 0x00, 0x00, 0x01, 0xaa }; /* SD card CMD8(0x1AA) */

    EXPECT_EQ(0, crc7_sd(0, buf, 0));
    EXPECT_EQ(0x4a, crc7_sd(0, buf, sizeof(buf)));
    EXPECT_EQ(0x43, crc7_sd(0, buf2, sizeof(buf2)));
}
