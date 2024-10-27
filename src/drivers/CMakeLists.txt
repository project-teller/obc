set(DRIVER_LIBRARIES lfs)
if(TARGET_MCU STREQUAL "POSIX")
    list(APPEND DRIVER_LIBRARIES lfsbd)
endif()

# Declare the generic module library
add_library(
    obc-drivers
    adc.cpp
    flashmem.cpp
    imu.cpp
    mag.cpp
    sdcard.cpp
    temperature.cpp
)
target_include_directories(
	obc-drivers PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_link_libraries(
    obc-drivers PUBLIC
    obc-core
    obc-hal
    ${DRIVER_LIBRARIES}
)
