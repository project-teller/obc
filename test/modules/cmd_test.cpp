#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

#include "core/telem/ack.h"
#include "core/telem/generic.h"
#include "core/utils/crc.h"
#include "hal/posix/system_debug.h"
#include "hal/posix/uart_debug.h"
#include "hal/system.h"
#include "hal/uart.h"
#include "modules/cmd.h"
#include "modules/log.h"
#include "modules/telem.h"
#include "modules/uart_rx.h"

using namespace teller;

#define PREAMBLE 0xca, 0xfe
#define GND_TO_OBC 0x12
#define OBC_TO_GND 0x21
#define SCM_TO_OBC 0x32
#define OBC_TO_SCM 0x23

class CmdTest : public testing::Test {
private:
    uint8_t responseBuffer[teller::telem::MAX_PAYLOAD_LENGTH];

protected:
    std::unique_ptr<hal::uart::UARTInputRedirector> inputRedirector;
    std::unique_ptr<hal::uart::UARTOutputRedirector> outputRedirector;

    void SetUp() override
    {
        hal::system::init();

        ASSERT_TRUE(hal::uart::init());
        ASSERT_TRUE(telem::init());
        ASSERT_TRUE(log::init());
        ASSERT_TRUE(uart_rx::init());
        ASSERT_TRUE(cmd::init());

        inputRedirector = std::make_unique<hal::uart::UARTInputRedirector>(hal::uart::RXSM);
        outputRedirector = std::make_unique<hal::uart::UARTOutputRedirector>(hal::uart::RXSM);
    }

    void TearDown() override
    {
        outputRedirector.reset();
        inputRedirector.reset();

        cmd::destroy();
        uart_rx::destroy();
        log::destroy();
        telem::destroy();
        hal::uart::destroy();
        hal::system::destroy();
    }

    bool processNextInboundCommand()
    {
        if (cmd::waiting()) {
            return cmd::processNext();
        } else {
            return false;
        }
    }

    bool processNextOutboundMessage()
    {
        if (telem::waiting()) {
            return telem::processNext();
        } else {
            return false;
        }
    }

    void feedMessage(uint8_t* message)
    {
        size_t length = message[5] + 8;

        /* Update CRC if necessary */
        if (message[length - 2] == 0 && message[length - 1] == 0) {
            uint16_t crc = crc_ccitt(0, message, length - 2);
            message[length - 2] = crc & 0xff;
            message[length - 1] = crc >> 8;
        }

        ASSERT_FALSE(uart_rx::read(hal::uart::RXSM));
        ASSERT_FALSE(processNextInboundCommand());
        inputRedirector->feed(std::string(reinterpret_cast<char*>(message), length));
        for (size_t i = 0; i < length - 1; i++) {
            ASSERT_FALSE(uart_rx::read(hal::uart::RXSM));
            ASSERT_FALSE(processNextInboundCommand());
        }
        ASSERT_TRUE(uart_rx::read(hal::uart::RXSM));
        ASSERT_TRUE(processNextInboundCommand());
    }

    void expectAck(telem::frames::frame_type_t frame_type, uint8_t expected_seq_no = 0, uint8_t seq_no = 0)
    {
        telem::frames::ack_data_t data = {
            .frame_type = frame_type,
            .seq_no = expected_seq_no,
            .result = telem::frames::ACK_ACCEPTED,
            .error = 0
        };
        expectAckFrame(&data, seq_no);
    }

    void expectAckFrame(const telem::frames::ack_data_t* data, uint8_t seq_no = 0)
    {
        uint8_t payload[64];
        uint8_t buffer[64];
        size_t payload_length = telem::frames::encodeAckFrame(data, payload);
        telem::envelope_t envelope = {
            .seq_no = seq_no,
            .frame_type = telem::frames::ACK,
            .source = telem::ONBOARD_COMPUTER,
            .target = telem::GROUND_STATION
        };
        size_t length = telem::serialize(
            buffer, sizeof(buffer), envelope, payload, payload_length);

        ASSERT_TRUE(processNextOutboundMessage());
        ASSERT_EQ(
            std::string(reinterpret_cast<char*>(buffer), length),
            outputRedirector->getAndClear());
    }

    void expectWarning(const std::string& message, uint8_t seq_no = 0)
    {
        std::ostringstream stream;
        std::string str;
        uint16_t crc;

        stream << "\xca\xfe";
        stream.put(seq_no);
        stream.put(2);
        stream.put(OBC_TO_GND);
        stream.put(message.size() + 1);
        stream.put(0x0c);
        stream << message;
        str = stream.str();

        crc = crc_ccitt(0, reinterpret_cast<uint8_t*>(str.data()), str.size());
        stream.put(crc & 0xff);
        stream.put(crc >> 8);

        ASSERT_TRUE(processNextOutboundMessage());
        ASSERT_EQ(stream.str(), outputRedirector->getAndClear());
    }
};

TEST_F(CmdTest, readUnhandledPacket)
{
    uint8_t heartbeatMessage[] = {
        PREAMBLE, 0x01, 0x01, GND_TO_OBC, 0x0a, 0xd2, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x69
    };
    uint8_t expectedLogMessage[] = {
        PREAMBLE, 0x00, 0x02, OBC_TO_GND, 0x11, 0x0c,
        85, 110, 104, 97, 110, 100, 108, 101, 100, 32, 112, 107, 116, 58, 32, 49,
        0xb5, 0x6a
    };
    char* msg;
    size_t i;

    /* Feed a heartbeat message into the task */
    ASSERT_FALSE(uart_rx::read(hal::uart::RXSM));
    ASSERT_FALSE(processNextInboundCommand());
    msg = reinterpret_cast<char*>(heartbeatMessage);
    inputRedirector->feed(std::string(msg, 6));
    for (i = 0; i < 6; i++) {
        ASSERT_FALSE(uart_rx::read(hal::uart::RXSM));
        ASSERT_FALSE(processNextInboundCommand());
    }
    inputRedirector->feed(std::string(msg + 6, sizeof(heartbeatMessage) - 6));
    for (i = 0; i < sizeof(heartbeatMessage) - 7; i++) {
        ASSERT_FALSE(uart_rx::read(hal::uart::RXSM));
        ASSERT_FALSE(processNextInboundCommand());
    }
    ASSERT_TRUE(uart_rx::read(hal::uart::RXSM));
    ASSERT_TRUE(processNextInboundCommand());

    /* We expect to receive a warning message in response */
    ASSERT_TRUE(processNextOutboundMessage());
    msg = reinterpret_cast<char*>(expectedLogMessage);
    ASSERT_EQ(std::string(msg, sizeof(expectedLogMessage)), outputRedirector->getAndClear());
}

TEST_F(CmdTest, readPacketForOtherComponent)
{
    uint8_t dummyLogMessage[] = {
        PREAMBLE, 0x00, 0x02, OBC_TO_GND, 0x11, 0x0c,
        85, 110, 104, 97, 110, 100, 108, 101, 100, 32, 112, 107, 116, 58, 32, 49,
        0xb5, 0x6a
    };
    char* msg;
    size_t i;

    /* Feed a dummy log message into the task that was meant for another component */
    feedMessage(dummyLogMessage);

    /* We expect no response */
    ASSERT_EQ(std::string(""), outputRedirector->getAndClear());
}

TEST_F(CmdTest, readResetPacket)
{
    uint8_t resetMessage[] = { PREAMBLE, 0x00, 0x05, GND_TO_OBC, 0x00, 0xff, 0x4f };
    char* msg;

    /* Prevent HAL from resetting the system for sake of testing */
    hal::system::preventNextReset();

    /* Feed a reset message into the task */
    feedMessage(resetMessage);

    /* Reset should have been performed if we hadn't prevented it earlier */
    ASSERT_EQ(1, hal::system::countPreventedResetAttempts());
}

TEST_F(CmdTest, readResetPacketFromForbiddenComponent)
{
    uint8_t resetMessage[] = { PREAMBLE, 0x00, 0x05, SCM_TO_OBC, 0x00, 0xcc, 0x6c };

    /* Prevent HAL from resetting the system for sake of testing */
    hal::system::preventNextReset();

    /* Feed a reset message from a forbidden component into the task */
    feedMessage(resetMessage);

    /* No reset should have been prevented */
    ASSERT_EQ(0, hal::system::countPreventedResetAttempts());
    expectWarning("Ignored reset req from c3");
}

TEST_F(CmdTest, readStoragePacket)
{
    /* clang-format off */
    uint8_t storageNopMessage[] = {
        /* NOP storage command addressed at area 1 */
        PREAMBLE, 0x00, 0x06, GND_TO_OBC, 0x07, 0x10, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    /* clang-format on */

    char* msg;
    size_t i;

    /* Feed a message into the task */
    feedMessage(storageNopMessage);
    expectAck(telem::frames::STORAGE);
}

TEST_F(CmdTest, readStoragePacketFromForbiddenComponent)
{
    /* clang-format off */
    uint8_t storageNopMessage[] = {
        /* NOP storage command addressed at area 1, but from the SCM */
        PREAMBLE, 0x00, 0x06, SCM_TO_OBC, 0x07, 0x10, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    /* clang-format on */

    /* Feed a reset message from a forbidden component into the task */
    feedMessage(storageNopMessage);
    expectWarning("Ignored storage req from c3");
}
