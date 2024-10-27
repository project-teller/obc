#include <cstring>
#include <gtest/gtest.h>

#include "modules/storage.h"
#include "modules/storage_posix_debug.h"
#include "modules/telem.h"

using namespace teller;

class StorageTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(telem::init());
        storage::removeAllFiles();
        ASSERT_TRUE(storage::init());

        storage::setup(/* format = */ storage::INIT_MODE_FORMAT);
    }

    void TearDown() override
    {
        storage::destroy();
        storage::removeAllFiles();
        telem::destroy();
    }
};

TEST_F(StorageTest, mountingUnmounting)
{
    EXPECT_TRUE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_FLASH_MEMORY));
    EXPECT_TRUE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_SD_CARD));
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_OK, teller::storage::getSubsystemStatus());

    EXPECT_EQ(0, teller::storage::unmountStorage(teller::telem::STORAGE_AREA_FLASH_MEMORY));

    EXPECT_FALSE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_FLASH_MEMORY));
    EXPECT_TRUE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_SD_CARD));
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_WARNING, teller::storage::getSubsystemStatus());

    EXPECT_EQ(0, teller::storage::unmountStorage(teller::telem::STORAGE_AREA_SD_CARD));

    EXPECT_FALSE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_FLASH_MEMORY));
    EXPECT_FALSE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_SD_CARD));
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_CRITICAL, teller::storage::getSubsystemStatus());

    EXPECT_EQ(0, teller::storage::mountStorage(teller::telem::STORAGE_AREA_FLASH_MEMORY));

    EXPECT_TRUE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_FLASH_MEMORY));
    EXPECT_FALSE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_SD_CARD));
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_WARNING, teller::storage::getSubsystemStatus());

    EXPECT_EQ(0, teller::storage::mountStorage(teller::telem::STORAGE_AREA_SD_CARD));

    EXPECT_TRUE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_FLASH_MEMORY));
    EXPECT_TRUE(teller::storage::isStorageMounted(teller::telem::STORAGE_AREA_SD_CARD));
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_OK, teller::storage::getSubsystemStatus());
}

TEST_F(StorageTest, erase)
{
    teller::telem::storage_area_t area = teller::telem::STORAGE_AREA_FLASH_MEMORY;

    EXPECT_TRUE(teller::storage::isStorageMounted(area));
    EXPECT_EQ(EIO, teller::storage::eraseStorage(area));
    EXPECT_TRUE(teller::storage::isStorageMounted(area));

    EXPECT_EQ(0, teller::storage::unmountStorage(area));

    EXPECT_FALSE(teller::storage::isStorageMounted(area));
    EXPECT_EQ(0, teller::storage::eraseStorage(area));
    EXPECT_FALSE(teller::storage::isStorageMounted(area));

    EXPECT_EQ(0, teller::storage::mountStorage(area));

    EXPECT_TRUE(teller::storage::isStorageMounted(area));
}

TEST_F(StorageTest, markErrored)
{
    teller::telem::storage_area_t area = teller::telem::STORAGE_AREA_FLASH_MEMORY;

    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_OK, teller::storage::getSubsystemStatus());
    teller::storage::markStorageAsErrored(teller::telem::NUM_STORAGE_AREAS);
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_OK, teller::storage::getSubsystemStatus());

    teller::storage::markStorageAsErrored(area);
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_ERROR, teller::storage::getSubsystemStatus());

    /* Filesystem can be unmounted even if errored */
    EXPECT_EQ(0, teller::storage::unmountStorage(area));
    EXPECT_FALSE(teller::storage::isStorageMounted(area));
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_ERROR, teller::storage::getSubsystemStatus());

    /* Filesystem can not be mounted when errored */
    EXPECT_EQ(EIO, teller::storage::mountStorage(area));
    EXPECT_EQ(teller::telem::SUBSYSTEM_STATUS_ERROR, teller::storage::getSubsystemStatus());
}

TEST_F(StorageTest, invalidArea)
{
    teller::storage::markStorageAsErrored(teller::telem::NUM_STORAGE_AREAS);

    EXPECT_EQ(EINVAL, teller::storage::mountStorage(teller::telem::NUM_STORAGE_AREAS));
    EXPECT_EQ(EINVAL, teller::storage::unmountStorage(teller::telem::NUM_STORAGE_AREAS));
    EXPECT_EQ(EINVAL, teller::storage::eraseStorage(teller::telem::NUM_STORAGE_AREAS));
}

TEST_F(StorageTest, convertLittleFSErrorCode)
{
    EXPECT_EQ(0, teller::storage::convertLittleFSErrorCode({}));
    EXPECT_EQ(EBADF, teller::storage::convertLittleFSErrorCode(littlefs::Error::BADF));
    EXPECT_EQ(EEXIST, teller::storage::convertLittleFSErrorCode(littlefs::Error::EXIST));
    EXPECT_EQ(EFBIG, teller::storage::convertLittleFSErrorCode(littlefs::Error::FBIG));
    EXPECT_EQ(EINVAL, teller::storage::convertLittleFSErrorCode(littlefs::Error::INVAL));
    EXPECT_EQ(EIO, teller::storage::convertLittleFSErrorCode(littlefs::Error::IO));
    EXPECT_EQ(EISDIR, teller::storage::convertLittleFSErrorCode(littlefs::Error::ISDIR));
    EXPECT_EQ(ENAMETOOLONG, teller::storage::convertLittleFSErrorCode(littlefs::Error::NAMETOOLONG));
    EXPECT_EQ(ENOENT, teller::storage::convertLittleFSErrorCode(littlefs::Error::NO_DD_ENTRY));
    EXPECT_EQ(ENOENT, teller::storage::convertLittleFSErrorCode(littlefs::Error::NO_FD_ENTRY));
    EXPECT_EQ(ENOENT, teller::storage::convertLittleFSErrorCode(littlefs::Error::NOENT));
    EXPECT_EQ(ENOMEM, teller::storage::convertLittleFSErrorCode(littlefs::Error::NOMEM));
    EXPECT_EQ(ENOSPC, teller::storage::convertLittleFSErrorCode(littlefs::Error::NOSPC));
    EXPECT_EQ(ENOTDIR, teller::storage::convertLittleFSErrorCode(littlefs::Error::NOTDIR));
    EXPECT_EQ(ENOTEMPTY, teller::storage::convertLittleFSErrorCode(littlefs::Error::NOTEMPTY));
    EXPECT_EQ(EIO, teller::storage::convertLittleFSErrorCode(static_cast<littlefs::Error>(123)));
}
