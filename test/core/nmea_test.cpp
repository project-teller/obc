#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/nmea/parser.h"

using namespace std;
using namespace teller::nmea;
using namespace testing;

const char validNMEAMessage[] = "$CNT,42,0,0,34,0,0,17,0,0,1*6F\r\n";

TEST(GMMTest, parsingValidMessage)
{
    Parser parser;
    size_t i, n = strlen(validNMEAMessage);
    const uint8_t junk[] = { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xca, 0xca, 0xca, 0x0b, 0xad };
    const uint8_t asciiJunk[] = { 'd', 'e', 'a', 'd', 'b', 'e', 'e', 'f' };
    const char* message;

    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 2, parser.feed(validNMEAMessage[i]));
        if (i == n - 2) {
            message = parser.getMessage();
            EXPECT_TRUE(std::memcmp(message, validNMEAMessage, strlen(validNMEAMessage) - 2) == 0);
        }
    }

    /* Add some junk bytes and then parse again */
    for (i = 0; i < sizeof(junk); i++) {
        ASSERT_FALSE(parser.feed(junk[i]));
    }
    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 2, parser.feed(validNMEAMessage[i]));
        if (i == n - 2) {
            message = parser.getMessage();
            EXPECT_TRUE(std::memcmp(message, validNMEAMessage, strlen(validNMEAMessage) - 2) == 0);
        }
    }

    /* Add some ASCII junk bytes and then parse again */
    for (i = 0; i < sizeof(asciiJunk); i++) {
        ASSERT_FALSE(parser.feed(asciiJunk[i]));
    }
    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 2, parser.feed(validNMEAMessage[i]));
        if (i == n - 2) {
            message = parser.getMessage();
            EXPECT_TRUE(std::memcmp(message, validNMEAMessage, strlen(validNMEAMessage) - 2) == 0);
        }
    }
}

TEST(GMMTest, parsingDollarSignRestartsParsing)
{
    Parser parser;
    size_t i, n = strlen(validNMEAMessage);
    const char* message;

    for (i = 0; i < 10; i++) {
        ASSERT_FALSE(parser.feed(validNMEAMessage[i]));
    }

    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 2, parser.feed(validNMEAMessage[i]));
        if (i == n - 2) {
            message = parser.getMessage();
            EXPECT_TRUE(std::memcmp(message, validNMEAMessage, strlen(validNMEAMessage) - 2) == 0);
        }
    }
}

TEST(GMMTest, parsingTooLongPayload)
{
    Parser parser;
    size_t i, n = strlen(validNMEAMessage);
    const char* message;

    for (i = 0; i < n - 5; i++) {
        ASSERT_FALSE(parser.feed(validNMEAMessage[i]));
    }
    for (i = 5; i < n - 5; i++) {
        ASSERT_FALSE(parser.feed(validNMEAMessage[i]));
    }
    for (i = 5; i < n - 5; i++) {
        ASSERT_FALSE(parser.feed(validNMEAMessage[i]));
    }
    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 2, parser.feed(validNMEAMessage[i]));
        if (i == n - 2) {
            message = parser.getMessage();
            EXPECT_TRUE(std::memcmp(message, validNMEAMessage, strlen(validNMEAMessage) - 2) == 0);
        }
    }
}

TEST(GMMTest, parsingPayloadExactlyAtLimit)
{
    const char* dummyMessage = "$CNT,foo,bar,baz,frob,CN,foo,bar,baz,frob,CN,foo,bar,baz,frob*2E\r\n";
    Parser parser;
    size_t i, n = strlen(dummyMessage);
    const char* message;

    for (i = 0; i < n; i++) {
        ASSERT_EQ(i == n - 2, parser.feed(dummyMessage[i]));
        if (i == n - 2) {
            message = parser.getMessage();
            EXPECT_TRUE(std::memcmp(message, dummyMessage, strlen(dummyMessage) - 2) == 0);
        }
    }
}

TEST(GMMTest, parsingInvalidCRC)
{
    Parser parser;
    size_t i, n = sizeof(validNMEAMessage);
    const char* message;

    for (i = 0; i < n - 3; i++) {
        ASSERT_FALSE(parser.feed(validNMEAMessage[i]));
    }
    ASSERT_FALSE(parser.feed(validNMEAMessage[n - 3] - 1));
    ASSERT_FALSE(parser.feed(validNMEAMessage[n - 2]));
    ASSERT_FALSE(parser.feed(validNMEAMessage[n - 1]));
}
