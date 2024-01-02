cmake_minimum_required(VERSION 3.21)

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

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Prepare for fetching STM32Cube, CMSIS and HAL as needed
include(${CMAKE_TOOLCHAIN_FILE})
set(FETCHCONTENT_QUIET FALSE)

# Traverse subdirectories
add_subdirectory(lib)
add_subdirectory(src)
