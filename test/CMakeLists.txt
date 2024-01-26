# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Set target name
set(TARGET_NAME teller-obc-test)

enable_testing()

add_executable(
    ${TARGET_NAME}
    crc_test.cpp
)
target_include_directories(
    ${TARGET_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(
    ${TARGET_NAME}
    GTest::gtest_main
    obccore
)

include(GoogleTest)
gtest_discover_tests(${TARGET_NAME})
