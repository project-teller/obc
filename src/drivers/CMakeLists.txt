# Declare the generic module library
add_library(
    obc-drivers
    temperature.cpp
)
target_include_directories(
	obc-drivers PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_link_libraries(
    obc-drivers PUBLIC
    obc-core
    obc-hal
)
