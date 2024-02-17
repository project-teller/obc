# Declare core library that does not contain any hardware-specific elements
add_library(
    obc-core

    telem/generic.cpp
    telem/heartbeat.cpp
    telem/text_message.cpp
    telem/timesync.cpp

    utils/crc.c
    utils/time.c
)
target_include_directories(
	obc-core PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}/..
)

# We need to make sure we are using the right compiler flags when compiling
# for STM32 so let's depend on the appropriate target even if we don't need
# any code from there
if(TARGET_MCU_CMSIS_MODULE)
    target_link_libraries(obc-core PRIVATE CMSIS::${TARGET_MCU_CMSIS_MODULE})
endif()
