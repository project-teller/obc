#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/telem/ack.h"
#include "core/telem/calibration.h"
#include "core/telem/clock_status.h"
#include "core/telem/generic.h"
#include "core/telem/heartbeat.h"
#include "core/telem/imu.h"
#include "core/telem/parser.h"
#include "core/telem/storage.h"
#include "core/telem/text_message.h"

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
        .mode = OBC_MODE_MISSION,
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
        },
        .lclStatusBits = {
            .gmm = true,
            .scm = false,
            .suc1 = true,
            .suc2 = true,
            .suc3 = false,
            .hvpsu = false
        }
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = { 80, 212, 18, 0, 42, 42, 214, 6, 27, 3, 13 };
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
    frames::text_message_data_t textMessage = {
        .module = MODULE_ID_SCM,
        .level = LOG_LEVEL_NOTICE,
        /* This weird initializer is needed to make things with with GCC 10,
         * see: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55227 */
        .message = {
            'O', 'n', 'e', ' ', 'd', 'o', 'e', 's', ' ', 'n', 'o', 't', ' ',
            's', 'i', 'm', 'p', 'l', 'y', ' ', 'w', 'a', 'l', 'k', ' ',
            'i', 'n', 't', 'o', ' ', 'M', 'o', 'r', 'd', 'o', 'r', 0
        }
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = {
        29, 79, 110, 101, 32, 100, 111, 101, 115, 32, 110, 111, 116, 32, 115,
        105, 109, 112, 108, 121, 32, 119, 97, 108, 107, 32, 105, 110, 116, 111,
        32, 77, 111, 114, 100, 111, 114
    };
    frames::text_message_data_t decoded;
    /* clang-format on */

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeTextMessageFrame(&textMessage, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeTextMessageFrame(encoded, sizeof(expectedBytes), &decoded);

    EXPECT_EQ(decoded.level, textMessage.level);
    EXPECT_EQ(decoded.module, textMessage.module);
    EXPECT_STREQ(decoded.message, textMessage.message);
}

TEST(TelemetryTest, clockStatusFrameEncoding)
{
    /* clang-format off */
    frames::clock_status_data_t message = {
        .timestampInMsec = 1234567,
        .rtcTimestampInMsec = 1707048820951,
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = {
        135, 214, 18, 0, 215, 224, 9, 116, 141, 1, 0, 0
    };
    frames::clock_status_data_t decoded;
    /* clang-format on */

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeClockStatusFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeClockStatusFrame(encoded, &decoded);

    EXPECT_EQ(decoded.timestampInMsec, message.timestampInMsec);
    EXPECT_EQ(decoded.rtcTimestampInMsec, message.rtcTimestampInMsec);
}

TEST(TelemetryTest, ackFrameEncoding)
{
    frames::ack_data_t message = {
        .frame_type = frames::CLOCK_STATUS,
        .seq_no = 64,
        .result = frames::NAK_FAILED,
        .error = 42
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = { 3, 64, 2, 42, 0 };
    frames::ack_data_t decoded;

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeAckFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeAckFrame(encoded, &decoded);

    EXPECT_EQ(decoded.frame_type, message.frame_type);
    EXPECT_EQ(decoded.seq_no, message.seq_no);
    EXPECT_EQ(decoded.result, message.result);
    EXPECT_EQ(decoded.error, message.error);

    message.result = frames::NUM_ACK_RESULT_CODES;
    expectedBytes[2] = frames::NAK_INVALID;
    EXPECT_EQ(sizeof(expectedBytes), frames::encodeAckFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    encoded[2] = 123;
    frames::decodeAckFrame(encoded, &decoded);
    EXPECT_EQ(decoded.frame_type, message.frame_type);
    EXPECT_EQ(decoded.seq_no, message.seq_no);
    EXPECT_EQ(decoded.result, frames::NAK_INVALID);
    EXPECT_EQ(decoded.error, message.error);
}

TEST(TelemetryTest, storageCommandFrameEncoding)
{
    frames::storage_command_data_t message = {
        .area = STORAGE_AREA_SD_CARD,
        .command = frames::STORAGE_COMMAND_READ,
        .offset = 1234,
        .length = 5678,
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = { 0x24, 0xd2, 0x04, 0x00, 0x00, 0x2e, 0x16 };
    frames::storage_command_data_t decoded;

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeStorageCommandFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeStorageCommandFrame(encoded, &decoded);

    EXPECT_EQ(decoded.area, message.area);
    EXPECT_EQ(decoded.command, message.command);
    EXPECT_EQ(decoded.offset, message.offset);
    EXPECT_EQ(decoded.length, message.length);

    message.length = 8192;
    expectedBytes[5] = expectedBytes[6] = 0;
    EXPECT_EQ(sizeof(expectedBytes), frames::encodeStorageCommandFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));
    frames::decodeStorageCommandFrame(encoded, &decoded);

    EXPECT_EQ(decoded.area, message.area);
    EXPECT_EQ(decoded.command, message.command);
    EXPECT_EQ(decoded.offset, message.offset);
    EXPECT_EQ(decoded.length, message.length);

    encoded[0] = 0xff;
    frames::decodeStorageCommandFrame(encoded, &decoded);
    EXPECT_EQ(decoded.area, STORAGE_AREA_UNKNOWN);
    EXPECT_EQ(decoded.command, frames::STORAGE_COMMAND_NOP);
}

TEST(TelemetryTest, imuFrameEncoding)
{
    /* clang-format off */
    frames::imu_data_t message = {
        .timestampInMsec = 0x12345678,
        .acceleration = { 1, 2, 3 },
        .angularVelocity = { 4, 5, 6 },
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = {
        0x78, 0x56, 0x34, 0x12,
        0x00, 0x00, 0x80, 0x3f,
        0x00, 0x00, 0x00, 0x40,
        0x00, 0x00, 0x40, 0x40,
        0x00, 0x00, 0x80, 0x40,
        0x00, 0x00, 0xa0, 0x40,
        0x00, 0x00, 0xc0, 0x40,
    };
    frames::imu_data_t decoded;
    /* clang-format on */

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeIMUFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeIMUFrame(encoded, &decoded);

    EXPECT_EQ(decoded.timestampInMsec, message.timestampInMsec);
    EXPECT_EQ(decoded.acceleration.x, message.acceleration.x);
    EXPECT_EQ(decoded.acceleration.y, message.acceleration.y);
    EXPECT_EQ(decoded.acceleration.z, message.acceleration.z);
    EXPECT_EQ(decoded.angularVelocity.x, message.angularVelocity.x);
    EXPECT_EQ(decoded.angularVelocity.y, message.angularVelocity.y);
    EXPECT_EQ(decoded.angularVelocity.z, message.angularVelocity.z);
}

TEST(TelemetryTest, calibrationRequestFrameEncoding)
{
    /* clang-format off */
    frames::calibration_request_data_t message = {
        .procedure = frames::CALIBRATION_GYRO,
    };
    uint8_t encoded[MAX_MESSAGE_LENGTH];
    uint8_t expectedBytes[] = { 0x01 };
    frames::calibration_request_data_t decoded;
    /* clang-format on */

    EXPECT_EQ(sizeof(expectedBytes), frames::encodeCalibrationRequestFrame(&message, encoded));
    EXPECT_EQ(0, memcmp(expectedBytes, encoded, sizeof(expectedBytes)));

    frames::decodeCalibrationRequestFrame(encoded, &decoded);

    EXPECT_EQ(decoded.procedure, message.procedure);
}

const uint8_t clockStatusMessage[] = {
    0xca, 0xfe, 0xd7, 0x03, 0x21, 0x0c, 0x37, 0xa4, 0x4b, 0x00, 0x00, 0x00,
    0x00, 0x88, 0x64, 0x37, 0x00, 0x00, 0x97, 0xa3
};

TEST(TelemetryTest, parsingValidMessage)
{
    Parser parser;
    size_t i, n = sizeof(clockStatusMessage);
    envelope_t envelope;
    const uint8_t junk[] = { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xca, 0xca, 0xca, 0x0b, 0xad };

    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 1, parser.feed(clockStatusMessage[i]));
    }

    envelope = parser.getEnvelope();
    EXPECT_EQ(0xd7, envelope.seq_no);
    EXPECT_EQ(frames::CLOCK_STATUS, envelope.frame_type);
    EXPECT_EQ(ONBOARD_COMPUTER, envelope.source);
    EXPECT_EQ(GROUND_STATION, envelope.target);

    /* Add some junk bytes and then parse again */
    for (i = 0; i < sizeof(junk); i++) {
        ASSERT_FALSE(parser.feed(junk[i]));
    }
    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 1, parser.feed(clockStatusMessage[i]));
    }

    envelope = parser.getEnvelope();
    EXPECT_EQ(0xd7, envelope.seq_no);
    EXPECT_EQ(frames::CLOCK_STATUS, envelope.frame_type);
    EXPECT_EQ(ONBOARD_COMPUTER, envelope.source);
    EXPECT_EQ(GROUND_STATION, envelope.target);

    for (i = teller::telem::HEADER_LENGTH; i < n - 2; i++) {
        EXPECT_EQ(clockStatusMessage[i], parser.getPayload()[i - teller::telem::HEADER_LENGTH]);
    }
}

TEST(TelemetryTest, parsingZeroLengthPayload)
{
    Parser parser;
    const uint8_t message[] = { 0xca, 0xfe, 0x00, 0x05, 0x12, 0x00, 0xff, 0x4f };
    size_t i, n = sizeof(message);
    envelope_t envelope;

    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 1, parser.feed(message[i]));
    }

    envelope = parser.getEnvelope();
    EXPECT_EQ(0x00, envelope.seq_no);
    EXPECT_EQ(frames::RESET, envelope.frame_type);
    EXPECT_EQ(GROUND_STATION, envelope.source);
    EXPECT_EQ(ONBOARD_COMPUTER, envelope.target);
}

TEST(TelemetryTest, parsingTooLongPayload)
{
    Parser parser;
    size_t i;

    for (i = 0; i < 5; i++) {
        ASSERT_FALSE(parser.feed(clockStatusMessage[i]));
    }
    ASSERT_FALSE(parser.feed(MAX_PAYLOAD_LENGTH + 1));
    ASSERT_EQ(teller::telem::ParserState::WAITING_SYNC_BYTE_1, parser.getState());
}

TEST(TelemetryTest, parsingInvalidCRC)
{
    Parser parser;
    size_t i, n = sizeof(clockStatusMessage);

    for (i = 0; i < n - 2; i++) {
        ASSERT_FALSE(parser.feed(clockStatusMessage[i]));
    }
    ASSERT_EQ(teller::telem::ParserState::READING_CHECKSUM, parser.getState());
    ASSERT_FALSE(parser.feed(0xff));
    ASSERT_EQ(teller::telem::ParserState::READING_CHECKSUM, parser.getState());
    ASSERT_FALSE(parser.feed(0xff));
    ASSERT_EQ(teller::telem::ParserState::WAITING_SYNC_BYTE_1, parser.getState());
}
