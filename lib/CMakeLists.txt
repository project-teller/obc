if(BUILD_TESTS)
    add_subdirectory(googletest)
endif()

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
