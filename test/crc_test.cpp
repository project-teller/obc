#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/crc.h"

// Demonstrate some basic assertions.
TEST(crc, basic)
{
    uint8_t buf[] = "123456789";
    size_t buf_len = std::strlen(reinterpret_cast<char*>(buf));

    EXPECT_EQ(0, crc_ccitt(0, buf, 0));
    EXPECT_EQ(0x2189, crc_ccitt(0, buf, buf_len));
    EXPECT_EQ(0xe026, crc_ccitt(0x2189, buf, buf_len));
}
