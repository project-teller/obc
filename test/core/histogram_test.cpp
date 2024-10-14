#include <cstring>
#include <gtest/gtest.h>

#include "core/utils/histogram.h"

/* clang-format off */
/* Each histogram example in the probes array ends with UINT16_MAX */
static const uint16_t probes[] = {
    UINT16_MAX,
    0, UINT16_MAX,
    0, 0, 0, 0, UINT16_MAX,
    0, 0, 42, 0, 0, UINT16_MAX,
    1337, 0, 42, 0, 0, UINT16_MAX,
    1337, 0, 42, 0, 0, 0, 0, 840, UINT16_MAX,
    4096, 4097, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4098, 4099, UINT16_MAX
};
#define PROBE_ARRAY_LENGTH (sizeof(probes) / sizeof(uint16_t))
#define NUM_PROBES 7

static const uint8_t expected_packed_sizes[NUM_PROBES] = {
    0, 1, 2, 5, 6, 8, 14,
};

static const uint8_t expected_unpacked_sizes[NUM_PROBES] = {
    0, 1, 4, 5, 5, 8, 21,
};

static const uint8_t expected_packed_histograms[NUM_PROBES][16] = {
    {},
    { 0 },
    { 64, 132 },
    { 64, 130, 42, 64, 130 },
    { 74, 185, 0, 42, 64, 130 },
    { 74, 185, 0, 42, 64, 132, 70, 200 },
    { 96, 160, 128, 96, 160, 129, 64, 145, 96, 160, 130, 96, 160, 131 },
};

/* clang-format on */

const uint16_t* find_end_of_probe(const uint16_t* probe)
{
    while (probe >= probes && probe < probes + PROBE_ARRAY_LENGTH && *probe < UINT16_MAX) {
        probe++;
    }

    if (probe >= probes + PROBE_ARRAY_LENGTH) {
        return nullptr;
    } else {
        return probe;
    }
}

const uint16_t* find_next_probe(const uint16_t* probe)
{
    const uint16_t* end = find_end_of_probe(probe);
    return end < probes + PROBE_ARRAY_LENGTH - 1 ? end + 1 : nullptr;
}

TEST(HistogramTest, getPackedSize)
{
    uint8_t i = 0;

    for (const uint16_t* probe = probes; probe; probe = find_next_probe(probe), i++) {
        size_t size = find_end_of_probe(probe) - probe;
        EXPECT_EQ(histogram_get_packed_size(probe, size), expected_packed_sizes[i]);
    }
}

TEST(HistogramTest, getUnpackedSize)
{
    for (uint8_t i = 0; i < NUM_PROBES; i++) {
        EXPECT_EQ(histogram_get_unpacked_size(expected_packed_histograms[i], expected_packed_sizes[i]), expected_unpacked_sizes[i]);
    }

    /* Try the last one with a short buffer, expect zero result */
    EXPECT_EQ(histogram_get_unpacked_size(expected_packed_histograms[NUM_PROBES - 1], expected_packed_sizes[NUM_PROBES - 1] - 1), 0);
}

TEST(HistogramTest, packUnpackRoundtrip)
{
    uint8_t i = 0;
    uint8_t buffer[256];
    uint16_t histogram[128];

    for (const uint16_t* probe = probes; probe; probe = find_next_probe(probe), i++) {
        size_t size = find_end_of_probe(probe) - probe;

        EXPECT_EQ(histogram_pack(buffer, probe, size), buffer + expected_packed_sizes[i]);
        EXPECT_EQ(memcmp(buffer, expected_packed_histograms[i], expected_packed_sizes[i]), 0);

        EXPECT_EQ(histogram_unpack(buffer, histogram, expected_packed_sizes[i]), histogram + size);
        EXPECT_EQ(memcmp(histogram, probe, size), 0);
    }
}
