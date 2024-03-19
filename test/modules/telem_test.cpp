#include <cstring>
#include <gtest/gtest.h>

#include "core/telem/generic.h"
#include "hal/posix/uart_debug.h"
#include "hal/uart.h"
#include "modules/telem.h"

using namespace teller;

class TelemetryModuleTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(hal::uart::init());
        ASSERT_TRUE(telem::init());
    }

    void TearDown() override
    {
        telem::destroy();
        hal::uart::destroy();
    }
};

TEST_F(TelemetryModuleTest, sendRawMessage)
{
    hal::uart::UARTOutputRedirector redirect(hal::uart::TELEMETRY);
    uint8_t buf[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };
    const char* test_string = "spam ham bacon";

    /* Sending raw byte buffer */
    ASSERT_TRUE(telem::send(buf, sizeof(buf)));
    ASSERT_TRUE(telem::flushNext());
    EXPECT_EQ(std::string(reinterpret_cast<char*>(buf), sizeof(buf)), redirect.getAndClear());

    /* Sending string */
    ASSERT_TRUE(telem::send(test_string));
    ASSERT_TRUE(telem::flushNext());
    EXPECT_EQ(test_string, redirect.getAndClear());

    /* Sending null pointer -- we should not crash */
    ASSERT_TRUE(telem::send(nullptr, 12));
}

TEST_F(TelemetryModuleTest, sendWithEnvelope)
{
    hal::uart::UARTOutputRedirector redirect(hal::uart::TELEMETRY);
    const char* test_string = "spam ham bacon";
    const char* too_long_test_string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string expected("\xCA\xFE\0\x2!\xEspam ham bacon\xB8\xE3", 22);
    std::string expected_empty("\xCA\xFE\x01\x2!\x00\x8b\x43", 8);

    ASSERT_TRUE(
        telem::send(
            telem::frames::TEXT_MESSAGE,
            reinterpret_cast<const uint8_t*>(test_string),
            strlen(test_string)));
    ASSERT_TRUE(telem::flushNext());
    EXPECT_EQ(expected, redirect.getAndClear());

    /* Sending null payload -- we should not crash but send a message with
     * an empty payload instead */
    ASSERT_TRUE(telem::send(telem::frames::TEXT_MESSAGE, nullptr, 12));
    ASSERT_TRUE(telem::flushNext());
    EXPECT_EQ(expected_empty, redirect.getAndClear());

    /* Sending too large payload */
    ASSERT_FALSE(
        telem::send(
            telem::frames::TEXT_MESSAGE,
            reinterpret_cast<const uint8_t*>(too_long_test_string),
            strlen(too_long_test_string)));
}
