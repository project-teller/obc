# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Set target name
set(TARGET_NAME teller-obc-test)

add_executable(
    ${TARGET_NAME}
    core/crc_test.cpp
    core/majority_voter_test.cpp
    core/nmea_test.cpp
    core/running_mean_test.cpp
    core/telem_test.cpp
    core/time_test.cpp
    core/vector_test.cpp
    hal/posix_board_test.cpp
    hal/posix_gpio_test.cpp
    hal/posix_hal_test.cpp
    hal/posix_led_test.cpp
    hal/posix_rtc_test.cpp
    hal/posix_queue_test.cpp
    hal/posix_system_test.cpp
    hal/posix_uart_test.cpp
    hal/posix_watchdog_test.cpp
    modules/cmd_test.cpp
    modules/edr_test.cpp
    modules/errors_test.cpp
    modules/lcl_test.cpp
    modules/log_test.cpp
    modules/messages_test.cpp
    modules/rxsm_test.cpp
    modules/storage_test.cpp
    modules/telem_test.cpp
)
target_include_directories(
    ${TARGET_NAME}
    PRIVATE
    ${CMAKE_BINARY_DIR}/src
)
target_link_libraries(
    ${TARGET_NAME}
    GTest::gtest_main
    GTest::gmock_main
    obc-modules
)

include(GoogleTest)
gtest_discover_tests(${TARGET_NAME})
