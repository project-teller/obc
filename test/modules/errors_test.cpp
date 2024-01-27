#include <cstring>
#include <gtest/gtest.h>

#include "hal/led.h"
#include "modules/errors.h"

using namespace teller::errors;

class ErrorTest : public testing::Test {
protected:
    void SetUp() override
    {
        teller::hal::led::init();
        init();
    }

    void TearDown() override
    {
        destroy();
        teller::hal::led::destroy();
    }
};

TEST_F(ErrorTest, setAndClear)
{
    EXPECT_FALSE(hasAnyErrors());

    setError(SYSTEM_INIT_ERROR);
    EXPECT_TRUE(hasAnyErrors());
    EXPECT_TRUE(hasError(SYSTEM_INIT_ERROR));
    EXPECT_FALSE(hasError(NOT_ENOUGH_MEMORY));

    setError(SYSTEM_INIT_ERROR);
    EXPECT_TRUE(hasAnyErrors());
    EXPECT_TRUE(hasError(SYSTEM_INIT_ERROR));
    EXPECT_FALSE(hasError(NOT_ENOUGH_MEMORY));

    setError(NOT_ENOUGH_MEMORY);
    EXPECT_TRUE(hasAnyErrors());
    EXPECT_TRUE(hasError(SYSTEM_INIT_ERROR));
    EXPECT_TRUE(hasError(NOT_ENOUGH_MEMORY));

    clearError(SYSTEM_INIT_ERROR);
    EXPECT_TRUE(hasAnyErrors());
    EXPECT_FALSE(hasError(SYSTEM_INIT_ERROR));
    EXPECT_TRUE(hasError(NOT_ENOUGH_MEMORY));
}
