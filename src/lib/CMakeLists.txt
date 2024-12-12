# LittleFS library
add_library(
    lfs
    littlefs/lfs.c
    littlefs-cpp/lfs_util.c
    littlefs-cpp/littlefs-cpp.cpp
)
target_compile_options(lfs PRIVATE -Wno-unused-function)
target_include_directories(
    lfs PUBLIC littlefs littlefs-cpp
)
if(TARGET_MCU_FREERTOS_MODULE)
    target_compile_definitions(
        # We want to use pvPortMalloc() from FreeRTOS
        lfs PUBLIC LFS_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/lfs_config.h LFS_MALLOC=pvPortMalloc LFS_FREE=vPortFree LFS_USES_FREERTOS=1
    )
    target_include_directories(
        lfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. # because of FreeRTOSConfig.h
    )
    target_link_libraries(
        lfs PUBLIC ${FREERTOS_NAMESPACE}::${TARGET_MCU_FREERTOS_MODULE}
    )
endif()

# LittleFS in-memory and in-file block device emulation for tests and the
# POSIX HAL layer
add_library(
    lfsbd
    littlefs/bd/lfs_filebd.c
    littlefs/bd/lfs_rambd.c
)
target_compile_options(lfsbd PRIVATE -Wno-unused-variable)
target_include_directories(
    lfsbd PUBLIC littlefs/bd
)
target_link_libraries(
    lfsbd PRIVATE lfs
)
# add_compile_definitions(lfsbd LFS_YES_TRACE=1 LFS_FILEBD_YES_TRACE=1)

#############################################################################

# libsdlog library
add_subdirectory(sdlog)
if(TARGET_MCU_FREERTOS_MODULE)
    target_compile_definitions(sdlog PUBLIC SDLOG_MALLOC=pvPortMalloc SDLOG_FREE=vPortFree)
    target_compile_definitions(sdlog PRIVATE SDLOG_MEMORY_INC=${CMAKE_CURRENT_SOURCE_DIR}/sdlog_config.h)
    target_include_directories(
        sdlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..    # because of FreeRTOSConfig.h
    )
    target_link_libraries(
        sdlog PUBLIC ${FREERTOS_NAMESPACE}::${TARGET_MCU_FREERTOS_MODULE}
    )
endif()

#############################################################################

# lwrb library
add_subdirectory(lwrb)

#############################################################################

# minmea library. Even though it's CMake-based, we cannot use its CMakeLists.txt
# file directly because it insists on find_packege(Threads)
add_library(
    minmea
    minmea/minmea.c
)
if(NOT HAVE_TIMEGM)
    add_compile_definitions(minmea timegm=mktime)
endif()
target_include_directories(
    minmea PUBLIC minmea
)

#############################################################################

# STM32 USB CDC library
if(TARGET_MCU_CMSIS_MODULE)
    add_subdirectory(usb-cdc)
endif()

#############################################################################

# We need to make sure we are using the right compiler flags when compiling
# for STM32 so let's depend on the appropriate target even if we don't need
# any code from there
if(TARGET_MCU_CMSIS_MODULE)
    target_link_libraries(lfs PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
    target_link_libraries(lfsbd PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
    target_link_libraries(lwrb PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
    target_link_libraries(minmea PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
    target_link_libraries(sdlog PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
    target_link_libraries(stm32-usbd-core PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
    target_link_libraries(stm32-usbd-cdc PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
endif()
