# Map TELLER_BOARD to TARGET_MCU
if(TELLER_BOARD STREQUAL "KakuteH7v2")
    set(TARGET_MCU "STM32H7")
elseif(TELLER_BOARD STREQUAL "Nucleo144")
	set(TARGET_MCU "STM32H7")
	set(OPENOCD_BOARD "st_nucleo_h743zi")
else()
	set(TARGET_MCU "${TELLER_BOARD}")
endif()

string(TOUPPER "${TELLER_BOARD}" TELLER_BOARD_UPPERCASE)
set(TELLER_BOARD_${TELLER_BOARD_UPPERCASE} 1)

# Hardware-specific stuff
if(TARGET_MCU STREQUAL "STM32H7")
	set(TARGET_MCU_FAMILY H7)
	set(TARGET_MCU_FREERTOS_MODULE ARM_CM7)
	set(TARGET_MCU_CMSIS_MODULE STM32::H743VI::M7)
elseif(TARGET_MCU STREQUAL "STM32F4")
	set(TARGET_MCU_FAMILY F4)
	set(TARGET_MCU_FREERTOS_MODULE ARM_CM4F)
	set(TARGET_MCU_CMSIS_MODULE STM32::F405RG)
elseif(TARGET_MCU STREQUAL "POSIX")
	# Nothing to do here
elseif(TARGET_MCU)
	message(FATAL_ERROR "Unsupported target board: ${TARGET_MCU}")
else()
	message(FATAL_ERROR "TARGET_MCU must be set before attempting a build")
endif()

# Create configuration file
configure_file(config.h.in config.h)

# Figure out which components to link to
if(BUILD_FIRMWARE)
	set(HAL_COMP_LIST RCC GPIO UART CORTEX DMA IWDG)
	set(CMSIS_COMP_LIST RTOS_V2)
else()
	set(HAL_COMP_LIST "")
	set(CMSIS_COMP_LIST "")
endif()
set(FREERTOS_COMP_LIST "")

# Hardware-specific stuff
if(TARGET_MCU STREQUAL "STM32H7")
	list(APPEND CMSIS_COMP_LIST STM32H743VI_M7)
	list(APPEND HAL_COMP_LIST STM32H7M7)
	list(APPEND FREERTOS_COMP_LIST ARM_CM7 STM32H7)

	set(CMSIS_NAMESPACE CMSIS::STM32::H7::M7)
	set(FREERTOS_NAMESPACE FreeRTOS::STM32::H7::M7)
	set(HAL_NAMESPACE HAL::STM32::H7::M7)
	set(CMSIS_RTOS_V2_DEVICE_HEADER stm32h7xx_hal.h)
elseif(TARGET_MCU STREQUAL "STM32F4")
	list(APPEND CMSIS_COMP_LIST STM32F405RG)
	list(APPEND HAL_COMP_LIST STM32F405RG)
	list(APPEND FREERTOS_COMP_LIST ARM_CM4F STM32F4)

	set(CMSIS_NAMESPACE CMSIS::STM32::F4)
	set(FREERTOS_NAMESPACE FreeRTOS::STM32::F4)
	set(HAL_NAMESPACE HAL::STM32::F4)
	set(CMSIS_RTOS_V2_DEVICE_HEADER stm32f4xx_hal.h)
endif()

# Fetch STM32Cube, CMSIS and HAL
if(TARGET_MCU_FAMILY)
	# We are compiling for a specific STM32 MCU so fetch all the dependencies
	# related to this MCU
	stm32_fetch_cube(${TARGET_MCU_FAMILY})
endif()

# Find CMSIS, HAL and FreeRTOS
if(CMSIS_COMP_LIST)
	find_package(CMSIS COMPONENTS ${CMSIS_COMP_LIST} REQUIRED)
endif()
if(HAL_COMP_LIST)
	find_package(HAL COMPONENTS ${HAL_COMP_LIST} REQUIRED)
endif()
if(FREERTOS_COMP_LIST)
	find_package(FreeRTOS COMPONENTS ${FREERTOS_COMP_LIST} REQUIRED)
endif()

# Create libraries
add_subdirectory(core)
add_subdirectory(hal)

if(BUILD_FIRMWARE)
	# Set target name
	set(TARGET_NAME teller-obc)

	# Specify source files, include paths and libraries to link to
	add_executable(
		${TARGET_NAME}

		main.cpp

		modules/errors.cpp
		modules/telem.cpp

		tasks/blinker.cpp
		tasks/serial.cpp
		tasks/supervisor.cpp
		tasks/telem.cpp
	)
	target_include_directories(
		${TARGET_NAME} PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
	)
	target_link_libraries(
		${TARGET_NAME} PRIVATE
		obc-core
		obc-hal
	)

	stm32_print_size_of_target(${TARGET_NAME})
	stm32_generate_binary_file(${TARGET_NAME})
	stm32_generate_hex_file(${TARGET_NAME})

	add_programming_target(TARGET ${TARGET_NAME} BOARD ${OPENOCD_BOARD})
	add_simulator_target(TARGET ${TARGET_NAME})
endif()

if(BUILD_TESTS)
	# Set target name
	set(TARGET_NAME obc-test)

	# List all source files here that are needed for the unit tests but do
	# not depend on the hardware
	add_library(
		${TARGET_NAME}

		modules/errors.cpp
	)
	target_include_directories(
		${TARGET_NAME} PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
	)
	target_link_libraries(
		${TARGET_NAME} PRIVATE
		obc-hal
	)
	target_link_libraries(
		${TARGET_NAME} PUBLIC
		obc-core
	)
endif()
