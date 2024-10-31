if(TELLER_BOARD STREQUAL "POSIX")
  set(EXTRA_OBC_MODULE_SOURCES storage_posix_debug.cpp)
endif()

# Declare the generic module library
add_library(
    obc-modules
    cmd.cpp
    debug.cpp
    edr.cpp
    errors.cpp
    gmm.cpp
    imu.cpp
    lcl.cpp
    log.cpp
    mag.cpp
    messages.cpp
    mode.cpp
    rxsm.cpp
    scheduler.cpp
    scm.cpp
    storage.cpp
    supervisor.cpp
    telem.cpp
    uart_rx.cpp
    ${EXTRA_OBC_MODULE_SOURCES}
)

target_include_directories(
	obc-modules PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_link_libraries(
    obc-modules PUBLIC
    obc-core
    obc-drivers
    obc-hal
    lfs
    sdlog
)
