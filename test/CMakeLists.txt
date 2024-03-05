# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Set target name
set(TARGET_NAME teller-obc-test)

add_executable(
    ${TARGET_NAME}
    core/crc_test.cpp
    core/majority_voter_test.cpp
    core/telem_test.cpp
    core/time_test.cpp
    hal/posix_gpio_test.cpp
    hal/posix_led_test.cpp
    hal/posix_rcc_test.cpp
    hal/posix_rtc_test.cpp
    hal/posix_queue_test.cpp
    hal/posix_system_test.cpp
    hal/posix_watchdog_test.cpp
    modules/errors_test.cpp
    modules/messages_test.cpp
    modules/rxsm_test.cpp
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
