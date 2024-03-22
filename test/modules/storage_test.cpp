#include <cstring>
#include <gtest/gtest.h>

#include "hal/posix/storage_debug.h"
#include "hal/storage.h"
#include "modules/storage.h"

using namespace teller;

class StorageTest : public testing::Test {
protected:
    void SetUp() override
    {
        hal::storage::removeAllFiles();
        ASSERT_TRUE(hal::storage::init());
        ASSERT_TRUE(storage::init(/* format = */ true));
    }

    void TearDown() override
    {
        storage::destroy();
        hal::storage::destroy();
        hal::storage::removeAllFiles();
    }
};

TEST_F(StorageTest, getSubsystemStatus)
{
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_OK, teller::storage::getSubsystemStatus());
}
