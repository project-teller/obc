#include <cstring>
#include <gtest/gtest.h>

#include "core/telem/generic.h"
#include "hal/posix/uart_debug.h"
#include "hal/uart.h"
#include "modules/log.h"
#include "modules/telem.h"

using namespace teller;

class LogTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(hal::uart::init());
        ASSERT_TRUE(telem::init());
        ASSERT_TRUE(log::init());
    }

    void TearDown() override
    {
        log::destroy();
        telem::destroy();
        hal::uart::destroy();
    }
};

TEST_F(LogTest, sendRawMessage)
{
    hal::uart::UARTOutputRedirector redirect(hal::uart::TELEMETRY);

    ASSERT_TRUE(log::send(telem::MODULE_ID_GMM, telem::LOG_LEVEL_WARNING, "dummy message"));
    ASSERT_TRUE(telem::flushNext());

    EXPECT_EQ(
        std::string("\xca\xfe\x00\x02!\x0e\x14"
                    "dummy message\xc5M",
            22),
        redirect.getAndClear());
}

TEST_F(LogTest, getLogger)
{
    hal::uart::UARTOutputRedirector redirect(hal::uart::TELEMETRY);

    ASSERT_TRUE(log::getLogger(telem::MODULE_ID_GMM).warning("dummy message"));
    ASSERT_TRUE(telem::flushNext());

    EXPECT_EQ(
        std::string("\xca\xfe\x00\x02!\x0e\x14"
                    "dummy message\xc5M",
            22),
        redirect.getAndClear());
}
