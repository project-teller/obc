cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 NEW)

# Add etc/cmake to CMake's search path so we can put our private stuff there
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/etc/cmake)

include(BuildType)
include(UseCCacheWhenInstalled)

# Declare the project, its version number and language
project(
  teller-obc
  VERSION 0.1.0
  DESCRIPTION "Software for the OBC of the TELLER experiment"
  HOMEPAGE_URL https://project-teller.eu
  LANGUAGES CXX C ASM
)

# Include helper functions for hiding differences between compilers
include(compilers)

# Include helper functions for generating targets
include(targets)

# Specify the C++ standard.
# C++17 required because of nested namespaces and [[nodiscard]].
# C++20 required because of designated initializers.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Prepare for fetching STM32Cube, CMSIS and HAL as needed
if(CMAKE_TOOLCHAIN_FILE)
  include(${CMAKE_TOOLCHAIN_FILE})
endif()
set(FETCHCONTENT_QUIET FALSE)

# Check whether we need to build the firmware and the unit tests in the current
# configuration
if(TELLER_BOARD STREQUAL "POSIX")
  set(BUILD_TESTS YES)
else()
  set(BUILD_FIRMWARE YES)
endif()

enable_testing()

# Traverse subdirectories
add_subdirectory(lib)
add_subdirectory(src)

# Enable unit tests
if(BUILD_TESTS)
  option(TELLER_ENABLE_CODE_COVERAGE "Enable code coverage calculation" OFF)
  add_subdirectory(test)
endif()
