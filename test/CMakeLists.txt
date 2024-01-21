# Add Google Test to the project
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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
