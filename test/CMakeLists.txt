# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Set target name
set(TARGET_NAME teller-obc-test)

add_executable(
    ${TARGET_NAME}
    core/crc_test.cpp
    core/telem_test.cpp
    core/time_test.cpp
    hal/posix_led_test.cpp
    hal/posix_rtc_test.cpp
    hal/posix_system_test.cpp
    modules/errors_test.cpp
)
target_include_directories(
    ${TARGET_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(
    ${TARGET_NAME}
    GTest::gtest_main
    GTest::gmock_main
    obc-test
)

include(GoogleTest)
gtest_discover_tests(${TARGET_NAME})
