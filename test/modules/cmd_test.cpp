#include <cstring>
#include <gtest/gtest.h>

#include "core/telem/generic.h"
#include "hal/posix/system_debug.h"
#include "hal/posix/uart_debug.h"
#include "hal/system.h"
#include "hal/uart.h"
#include "modules/cmd.h"
#include "modules/log.h"
#include "modules/telem.h"

using namespace teller;

class CmdTest : public testing::Test {
private:
    uint8_t responseBuffer[teller::telem::MAX_PAYLOAD_LENGTH];

protected:
    void SetUp() override
    {
        ASSERT_TRUE(hal::uart::init());
        ASSERT_TRUE(telem::init());
        ASSERT_TRUE(log::init());
        ASSERT_TRUE(cmd::init());
    }

    void TearDown() override
    {
        cmd::destroy();
        log::destroy();
        telem::destroy();
        hal::uart::destroy();
    }

    bool handleCommands()
    {
        return cmd::handleCommands(responseBuffer);
    }
};

TEST_F(CmdTest, readUnhandledPacket)
{
    hal::uart::UARTInputRedirector inputRedirector(hal::uart::TELEMETRY);
    hal::uart::UARTOutputRedirector outputRedirector(hal::uart::TELEMETRY);
    uint8_t heartbeatMessage[] = {
        0xca, 0xfe, 0x01, 0x01, 0x12, 0x0a, 0xd2, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x69
    };
    uint8_t expectedLogMessage[] = {
        0xca, 0xfe, 0x00, 0x02, 0x21, 0x11, 0x0c,
        85, 110, 104, 97, 110, 100, 108, 101, 100, 32, 112, 107, 116, 58, 32, 49,
        0xb5, 0x6a
    };
    char* msg;
    size_t i;

    /* Feed a heartbeat message into the task */
    ASSERT_FALSE(handleCommands());
    msg = reinterpret_cast<char*>(heartbeatMessage);
    inputRedirector.feed(std::string(msg, 6));
    for (i = 0; i < 6; i++) {
        ASSERT_FALSE(handleCommands());
    }
    inputRedirector.feed(std::string(msg + 6, sizeof(heartbeatMessage) - 6));
    for (i = 0; i < sizeof(heartbeatMessage) - 7; i++) {
        ASSERT_FALSE(handleCommands());
    }
    ASSERT_TRUE(handleCommands());

    /* We expect to receive a warning message in response */
    telem::flushNext();
    msg = reinterpret_cast<char*>(expectedLogMessage);
    ASSERT_EQ(std::string(msg, sizeof(expectedLogMessage)), outputRedirector.getAndClear());
}

TEST_F(CmdTest, readPacketForOtherComponent)
{
    hal::uart::UARTInputRedirector inputRedirector(hal::uart::TELEMETRY);
    hal::uart::UARTOutputRedirector outputRedirector(hal::uart::TELEMETRY);
    uint8_t dummyLogMessage[] = {
        0xca, 0xfe, 0x00, 0x02, 0x21, 0x11, 0x0c,
        85, 110, 104, 97, 110, 100, 108, 101, 100, 32, 112, 107, 116, 58, 32, 49,
        0xb5, 0x6a
    };
    char* msg;
    size_t i;

    /* Feed a dummy log message into the task that was meant for another component */
    ASSERT_FALSE(handleCommands());
    msg = reinterpret_cast<char*>(dummyLogMessage);
    inputRedirector.feed(std::string(msg, sizeof(dummyLogMessage)));
    for (i = 0; i < sizeof(dummyLogMessage) - 1; i++) {
        ASSERT_FALSE(handleCommands());
    }
    ASSERT_TRUE(handleCommands());
}

TEST_F(CmdTest, readResetPacket)
{
    hal::uart::UARTInputRedirector inputRedirector(hal::uart::TELEMETRY);
    hal::uart::UARTOutputRedirector outputRedirector(hal::uart::TELEMETRY);
    uint8_t resetMessage[] = { 0xca, 0xfe, 0x00, 0x05, 0x12, 0x00, 0xff, 0x4f };
    char* msg;
    size_t i;

    /* Prevent HAL from resetting the system for sake of testing */
    hal::system::preventNextReset();

    /* Feed a reset message into the task */
    ASSERT_FALSE(handleCommands());
    msg = reinterpret_cast<char*>(resetMessage);
    inputRedirector.feed(std::string(msg, sizeof(resetMessage)));
    for (i = 0; i < sizeof(resetMessage) - 1; i++) {
        ASSERT_FALSE(handleCommands());
    }
    ASSERT_TRUE(handleCommands());

    /* Reset should have been performed if we hadn't prevented it earlier */
    ASSERT_EQ(1, hal::system::countPreventedResetAttempts());
}

TEST_F(CmdTest, readResetPacketFromForbiddenComponent)
{
    hal::uart::UARTInputRedirector inputRedirector(hal::uart::TELEMETRY);
    hal::uart::UARTOutputRedirector outputRedirector(hal::uart::TELEMETRY);
    uint8_t resetMessage[] = { 0xca, 0xfe, 0x00, 0x05, 0x32, 0x00, 0xcc, 0x6c };
    /* clang-format off */
    uint8_t expectedResponse[] = {
        0xca, 0xfe, 0x00, 0x02, 0x21, 0x1a, 0x0c,
        73, 103, 110, 111, 114, 101, 100, 32, 114, 101, 115, 101, 116, 32, 114,
        101, 113, 32, 102, 114, 111, 109, 32, 99, 51,
        0x5f, 0x34,
    };
    /* clang-format on */
    char* msg;
    size_t i;

    /* Prevent HAL from resetting the system for sake of testing */
    hal::system::preventNextReset();

    /* Feed a reset message from a forbidden component into the task */
    ASSERT_FALSE(handleCommands());
    msg = reinterpret_cast<char*>(resetMessage);
    inputRedirector.feed(std::string(msg, sizeof(resetMessage)));
    for (i = 0; i < sizeof(resetMessage) - 1; i++) {
        ASSERT_FALSE(handleCommands());
    }
    ASSERT_TRUE(handleCommands());

    /* No reset should have been prevented */
    ASSERT_EQ(0, hal::system::countPreventedResetAttempts());

    /* We expect to receive a warning message in response */
    telem::flushNext();
    msg = reinterpret_cast<char*>(expectedResponse);
    ASSERT_EQ(std::string(msg, sizeof(expectedResponse)), outputRedirector.getAndClear());
}
