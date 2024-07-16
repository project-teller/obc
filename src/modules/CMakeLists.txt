# Declare the generic module library
add_library(
    obc-modules
    cmd.cpp
    debug.cpp
    edr.cpp
    errors.cpp
    imu.cpp
    lcl.cpp
    log.cpp
    messages.cpp
    mode.cpp
    rxsm.cpp
    storage.cpp
    telem.cpp
)
target_include_directories(
	obc-modules PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_link_libraries(
    obc-modules PUBLIC
    obc-core
    obc-hal
    lfs
    sdlog
)
