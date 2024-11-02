# Declare core library that does not contain any hardware-specific elements
add_library(
    obc-core

    nmea/parser.cpp

    telem/ack.cpp
    telem/binary_data.cpp
    telem/calibration.cpp
    telem/clock_status.cpp
    telem/clock_sync.cpp
    telem/debug.cpp
    telem/echo.cpp
    telem/generic.cpp
    telem/gmm.cpp
    telem/heartbeat.cpp
    telem/imu.cpp
    telem/lcl_reset.cpp
    telem/mag.cpp
    telem/parser.cpp
    telem/scm.cpp
    telem/storage.cpp
    telem/text_message.cpp

    utils/block_cache.cpp
    utils/crc.c
    utils/histogram.c
    utils/majority_voter.cpp
    utils/random.c
    utils/smart_file_handle.cpp
    utils/time.c
    utils/varuint.c
)
target_link_libraries(
    obc-core PUBLIC lfs minmea
)
target_include_directories(
	obc-core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_include_directories(
    obc-core PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

# We need to make sure we are using the right compiler flags when compiling
# for STM32 so let's depend on the appropriate target even if we don't need
# any code from there
if(TARGET_MCU_CMSIS_MODULE)
    target_link_libraries(obc-core PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
endif()
