#include <gtest/gtest.h>

#include "modules/edr.hpp"
#include "modules/storage.h"

using namespace teller;

class EDRTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(storage::init());
        ASSERT_TRUE(edr::init());
    }

    void TearDown() override
    {
        edr::destroy();
        storage::destroy();
    }
};

TEST_F(EDRTest, dummy)
{
}
