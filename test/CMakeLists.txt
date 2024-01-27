# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Enable code coverage support
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND TELLER_ENABLE_CODE_COVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_gcovr_html(
        NAME coverage
        EXECUTABLE "${CMAKE_CTEST_COMMAND}" "--test-dir" "${PROJECT_BINARY_DIR}"
        DEPENDENCIES teller-obc-test
        EXCLUDE
        "/Applications/Xcode*"
        "/Library/Developer/*"
        "lib/googletest/*"
        "test/*"
    )
endif()

# Set target name
set(TARGET_NAME teller-obc-test)

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
    obc-core
)

include(GoogleTest)
gtest_discover_tests(${TARGET_NAME})
