#include <gtest/gtest.h>

#include "hal/posix/uart_debug.h"
#include "hal/uart.h"

using namespace teller::hal::uart;

class UARTTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(init());
    }

    void TearDown() override
    {
        destroy();
    }
};

TEST_F(UARTTest, read)
{
    UARTInputRedirector redirect(DEBUG);
    uint8_t buf[512];
    uint16_t bytes_read;

    EXPECT_TRUE(teller::hal::uart::readInto(DEBUG, buf, 0, nullptr));
    EXPECT_TRUE(teller::hal::uart::readInto(DEBUG, buf, 0, &bytes_read));
    EXPECT_EQ(0, bytes_read);

    redirect.feed("spam spam spam 42");

    EXPECT_TRUE(teller::hal::uart::readInto(DEBUG, buf, 6, &bytes_read));
    EXPECT_EQ(6, bytes_read);
    EXPECT_EQ(0, strncmp("spam s", reinterpret_cast<char*>(buf), bytes_read));

    EXPECT_TRUE(teller::hal::uart::readInto(DEBUG, buf, 6, nullptr));
    EXPECT_EQ(0, strncmp("pam sp", reinterpret_cast<char*>(buf), 6));

    EXPECT_TRUE(teller::hal::uart::readInto(DEBUG, buf, 16, &bytes_read));
    EXPECT_EQ(5, bytes_read);
    EXPECT_EQ(0, strncmp("am 42", reinterpret_cast<char*>(buf), bytes_read));

    EXPECT_FALSE(teller::hal::uart::readInto(DEBUG, buf, 16, nullptr));
    EXPECT_FALSE(teller::hal::uart::readInto(DEBUG, buf, 16, &bytes_read));
    EXPECT_EQ(0, bytes_read);

    redirect.feed("spam spam spam 42");

    EXPECT_TRUE(teller::hal::uart::readInto(DEBUG, buf, 6, &bytes_read));
    EXPECT_EQ(6, bytes_read);
    EXPECT_EQ(0, strncmp("spam s", reinterpret_cast<char*>(buf), bytes_read));

    redirect.clear();

    EXPECT_FALSE(teller::hal::uart::readInto(DEBUG, buf, 16, &bytes_read));
    EXPECT_EQ(0, bytes_read);
}

TEST_F(UARTTest, readFromSink)
{
    uint8_t buf[512];
    uint16_t bytes_read;

    EXPECT_TRUE(teller::hal::uart::readInto(SINK, buf, 0, nullptr));
    EXPECT_TRUE(teller::hal::uart::readInto(SINK, buf, 0, &bytes_read));
    EXPECT_EQ(0, bytes_read);

    EXPECT_FALSE(teller::hal::uart::readInto(SINK, buf, 4, &bytes_read));
    EXPECT_EQ(0, bytes_read);
}

TEST_F(UARTTest, writeString)
{
    UARTOutputRedirector redirect(DEBUG);
    write(DEBUG, "spam spam spam");
    EXPECT_EQ("spam spam spam", redirect.getAndClear());
}

TEST_F(UARTTest, writeStringToSink)
{
    write(SINK, "spam spam spam");
}

TEST_F(UARTTest, redirectorDoubleOverride)
{
    UARTInputRedirector redirectIn(DEBUG);
    UARTOutputRedirector redirectOut(DEBUG);

    EXPECT_THROW({ UARTInputRedirector dummy(DEBUG); }, std::runtime_error);

    EXPECT_THROW({ UARTOutputRedirector dummy(DEBUG); }, std::runtime_error);
}
