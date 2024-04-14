# LittleFS library
add_library(
    lfs
    littlefs/lfs.c
    littlefs/lfs_util.c
    littlefs-cpp/littlefs-cpp.cpp
)
target_include_directories(
    lfs PUBLIC littlefs littlefs-cpp
)

# LittleFS in-memory and in-file block device emulation for tests and the
# POSIX HAL layer
add_library(
    lfsbd
    littlefs/bd/lfs_filebd.c
    littlefs/bd/lfs_rambd.c
)
target_include_directories(
    lfsbd PUBLIC littlefs/bd
)
target_link_libraries(
    lfsbd PRIVATE lfs
)

#############################################################################

# libsdlog library
add_subdirectory(sdlog)

#############################################################################

# We need to make sure we are using the right compiler flags when compiling
# for STM32 so let's depend on the appropriate target even if we don't need
# any code from there
if(TARGET_MCU_CMSIS_MODULE)
    target_link_libraries(lfs PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
    target_link_libraries(lfsbd PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
endif()
