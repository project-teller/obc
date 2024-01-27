#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/telem.h"

using namespace std;
using namespace teller::telem;
using namespace testing;

TEST(TelemetryTest, getMessageSizeForPayloadLength)
{
    EXPECT_EQ(16, getMessageSizeForPayloadLength(8));
    EXPECT_EQ(20, getMessageSizeForPayloadLength(12));

    /* Oversized message */
    EXPECT_EQ(0, getMessageSizeForPayloadLength(64));
}

TEST(TelemetryTest, serialize)
{
    envelope_t envelope = {
        .seq_no = 42,
        .frame_type = frames::TEXT_MESSAGE,
        .source = GROUND_STATION,
        .target = ONBOARD_COMPUTER,
    };
    uint8_t payload[] = { 'h', 'e', 'l', 'l', 'o', '\n' };
    uint8_t result[80];
    uint8_t expected[] = {
        0xca, 0xfe, envelope.seq_no, frames::TEXT_MESSAGE, 0x12, sizeof(payload),
        'h', 'e', 'l', 'l', 'o', '\n', 0xfe, 0xbc
    };
    uint8_t serialized_length;

    /* Test serialization */
    serialized_length = serialize(result, sizeof(result), envelope, payload, sizeof(payload));
    EXPECT_EQ(
        getMessageSizeForPayloadLength(sizeof(payload)),
        serialized_length);

    {
        std::vector<uint8_t> vec(result, result + serialized_length);
        EXPECT_THAT(vec, ElementsAreArray(expected));
    }

    /* Test omitted source and target */
    envelope.source = UNKNOWN_COMPONENT;
    envelope.target = UNKNOWN_COMPONENT;

    serialized_length = serialize(result, sizeof(result), envelope, payload, sizeof(payload));
    EXPECT_EQ(
        getMessageSizeForPayloadLength(sizeof(payload)),
        serialized_length);

    {
        expected[4] = 0x21;
        expected[12] = 0xa6;
        expected[13] = 0xdb;
        std::vector<uint8_t> vec(result, result + serialized_length);
        EXPECT_THAT(vec, ElementsAreArray(expected));
    }
}

TEST(TelemetryTest, serializeInvalidPayload)
{
    envelope_t envelope = {
        .seq_no = 42,
        .frame_type = frames::TEXT_MESSAGE,
        .source = GROUND_STATION,
        .target = ONBOARD_COMPUTER,
    };
    uint8_t payload[] = { 'h', 'e', 'l', 'l', 'o', '\n' };
    uint8_t result[80];

    /* Test oversized payload */
    EXPECT_EQ(0, serialize(result, sizeof(result), envelope, payload, 80));

    /* Test too small buffer */
    EXPECT_EQ(0, serialize(result, 2, envelope, payload, sizeof(payload)));
}
