#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/varuint.h"

/* clang-format off */
static const uint32_t probes[] = {
    0, 1, 2, 62, 63,
    64, 65, 4094, 4095,
    4096, 4097, 262142, 262143,
    262144, 262145, 16777214, 16777215,
    16777216, 16777217, 1073741822, 1073741823,
    1073741824, 1073741825, 4294967294, 4294967295
};

static const uint8_t NUM_PROBES = sizeof(probes) / sizeof(probes[0]);

static const uint8_t expected_sizes[] = {
    1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};
/* clang-format on */

TEST(VaruintTest, getSize)
{
    for (uint8_t i = 0; i < NUM_PROBES; i++) {
        EXPECT_EQ(varuint_size(probes[i]), expected_sizes[i]);
        EXPECT_EQ(varuint_size_overlong(probes[i], 0), expected_sizes[i]);
        EXPECT_EQ(varuint_size_overlong(probes[i], 1), expected_sizes[i] + 1);
        EXPECT_EQ(varuint_size_overlong(probes[i], 2), expected_sizes[i] + 2);
    }
}

TEST(VaruintTest, encodeDecodeRoundtrip)
{
    uint8_t buf[16];
    uint8_t size;
    const uint8_t* end;
    uint32_t observed;
    uint8_t observed_overlong;

    for (uint8_t i = 0; i < NUM_PROBES; i++) {
        size = varuint_size(probes[i]);

        end = varuint_encode(buf, probes[i]);
        EXPECT_EQ(end, buf + size);

        end = varuint_decode(buf, &observed);
        EXPECT_EQ(end, buf + size);
        EXPECT_EQ(observed, probes[i]);
    }

    for (uint8_t i = 0; i < NUM_PROBES; i++) {
        size = varuint_size(probes[i]);
        for (uint8_t j = 0; j < 7 - size; j++) {
            end = varuint_encode_overlong(buf, probes[i], j);
            EXPECT_EQ(end, buf + size + j);

            end = varuint_decode(buf, &observed);
            EXPECT_EQ(end, buf + size + j);
            EXPECT_EQ(observed, probes[i]);

            end = varuint_decode_overlong(buf, &observed, &observed_overlong);
            EXPECT_EQ(end, buf + size + j);
            EXPECT_EQ(observed, probes[i]);
            EXPECT_EQ(observed_overlong, j);
        }
    }
}

TEST(VaruintTest, decodeErrors)
{
    const uint8_t* buf;
    uint32_t result;
    uint8_t overlong;
    int i;
    const uint8_t examples[][8] = {
        { 0xff },
        { 0x40, 0x00 },
        { 0x00 }
    };

    for (i = 0; examples[i][0] != 0; i++) {
        buf = examples[i];
        result = 0xdeadbeef;
        overlong = 0x42;
        EXPECT_EQ(buf, varuint_decode_overlong(buf, &result, &overlong));
        EXPECT_EQ(result, 0xdeadbeef);
        EXPECT_EQ(overlong, 0x42);
    }
}

TEST(VaruintTest, incrementalDecoding)
{
    uint8_t buf[16];
    uint8_t size;
    uint8_t* ptr;
    const uint8_t* end;
    uint32_t observed;
    uint8_t observed_overlong;
    varuint_decoder_t decoder;

    varuint_decoder_init(&decoder);

    for (uint8_t i = 0; i < NUM_PROBES; i++) {
        size = varuint_size(probes[i]);

        for (uint8_t j = 0; j < 7 - size; j++) {
            end = varuint_encode_overlong(buf, probes[i], j);
            EXPECT_EQ(end, buf + size + j);

            if (buf < end) {
                for (ptr = buf; ptr != end; ptr++) {
                    EXPECT_EQ(varuint_decoder_feed(&decoder, *ptr), ptr == end - 1);
                }

                EXPECT_EQ(probes[i], varuint_decoder_get_value(&decoder));
                EXPECT_EQ(j, varuint_decoder_get_overlong(&decoder));
            }
        }
    }

    varuint_decoder_destroy(&decoder);
}
