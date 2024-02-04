#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/telem/generic.h"
#include "core/telem/heartbeat.h"
#include "core/telem/text_message.h"
#include "core/telem/timesync.h"

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

TEST(TelemetryTest, heartbeatFrameEncoding)
{
    /* clang-format off */
    frames::heartbeat_data_t heartbeat = {
        .timestampInMsec = 1234000,
        .error = 42,
        .voltageInVolts = 4.2,
        .temperateInCelsius = -42.42,
        .rxsmStatusBits = {
            .sods = false,
            .soe = true,
            .lo = true
        },
        .subsystemStatus = {
            .gmm = SUBSYSTEM_STATUS_OK,
            .scm = SUBSYSTEM_STATUS_WARNING,
            .ads = SUBSYSTEM_STATUS_ERROR,
            .imu = SUBSYSTEM_STATUS_CRITICAL,
            .mag = SUBSYSTEM_STATUS_OK
        }
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = { 80, 212, 18, 0, 42, 42, 214, 6, 27, 3 };
    frames::heartbeat_data_t decoded;
    /* clang-format on */

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeHeartbeatFrame(&heartbeat, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeHeartbeatFrame(encoded, &decoded);

    EXPECT_EQ(decoded.timestampInMsec, heartbeat.timestampInMsec);
    EXPECT_EQ(decoded.error, heartbeat.error);
    EXPECT_EQ(decoded.voltageInVolts, heartbeat.voltageInVolts);
    EXPECT_EQ(decoded.temperateInCelsius, std::roundf(heartbeat.temperateInCelsius));
    EXPECT_EQ(decoded.rxsmStatusBits.lo, heartbeat.rxsmStatusBits.lo);
    EXPECT_EQ(decoded.rxsmStatusBits.sods, heartbeat.rxsmStatusBits.sods);
    EXPECT_EQ(decoded.rxsmStatusBits.soe, heartbeat.rxsmStatusBits.soe);
    EXPECT_EQ(decoded.subsystemStatus.gmm, heartbeat.subsystemStatus.gmm);
    EXPECT_EQ(decoded.subsystemStatus.scm, heartbeat.subsystemStatus.scm);
    EXPECT_EQ(decoded.subsystemStatus.ads, heartbeat.subsystemStatus.ads);
    EXPECT_EQ(decoded.subsystemStatus.imu, heartbeat.subsystemStatus.imu);
    EXPECT_EQ(decoded.subsystemStatus.mag, heartbeat.subsystemStatus.mag);
}

TEST(TelemetryTest, textMessageFrameEncoding)
{
    /* clang-format off */
    frames::text_message_data_t message = {
        .module = MODULE_ID_SCM,
        .level = LOG_LEVEL_NOTICE,
        .message = "One does not simply walk into Mordor"
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = {
        29, 79, 110, 101, 32, 100, 111, 101, 115, 32, 110, 111, 116, 32, 115,
        105, 109, 112, 108, 121, 32, 119, 97, 108, 107, 32, 105, 110, 116, 111,
        32, 77, 111, 114, 100, 111, 114
    };
    frames::text_message_data_t decoded;
    /* clang-format on */

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeTextMessageFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeTextMessageFrame(encoded, sizeof(expectedBytes), &decoded);

    EXPECT_EQ(decoded.level, message.level);
    EXPECT_EQ(decoded.module, message.module);
    EXPECT_STREQ(decoded.message, message.message);
}

TEST(TelemetryTest, timesyncFrameEncoding)
{
    /* clang-format off */
    frames::timesync_data_t message = {
        .timestampInMsec = 1234567,
        .rtcTimestampInMsec = 1707048820951,
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = {
        135, 214, 18, 0, 215, 224, 9, 116, 141, 1, 0, 0
    };
    frames::timesync_data_t decoded;
    /* clang-format on */

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeTimesyncFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeTimesyncFrame(encoded, &decoded);

    EXPECT_EQ(decoded.timestampInMsec, message.timestampInMsec);
    EXPECT_EQ(decoded.rtcTimestampInMsec, message.rtcTimestampInMsec);
}
