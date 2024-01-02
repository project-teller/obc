# Map TELLER_BOARD to TARGET_MCU
if(TELLER_BOARD STREQUAL "KakuteH7v2")
    set(TARGET_MCU "STM32H7")
else()
	set(TARGET_MCU "${TELLER_BOARD}")
endif()

string(TOUPPER "${TELLER_BOARD}" TELLER_BOARD_UPPERCASE)
set(TELLER_BOARD_${TELLER_BOARD_UPPERCASE} 1)

# Create configuration file
configure_file(config.h.in config.h)

# Figure out which components to link to
set(HAL_COMP_LIST RCC GPIO UART CORTEX)
set(CMSIS_COMP_LIST RTOS_V2)
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

	set(TARGET_MCU_FAMILY H7)
	set(TARGET_MCU_FREERTOS_MODULE ARM_CM7)
	set(TARGET_MCU_CMSIS_MODULE STM32::H743VI::M7)
elseif(TARGET_MCU STREQUAL "STM32F4")
	list(APPEND CMSIS_COMP_LIST STM32F405RG)
	list(APPEND HAL_COMP_LIST STM32F405RG)
	list(APPEND FREERTOS_COMP_LIST ARM_CM4F STM32F4)

	set(CMSIS_NAMESPACE CMSIS::STM32::F4)
	set(FREERTOS_NAMESPACE FreeRTOS::STM32::F4)
	set(HAL_NAMESPACE HAL::STM32::F4)
	set(CMSIS_RTOS_V2_DEVICE_HEADER stm32f4xx_hal.h)

	set(TARGET_MCU_FAMILY F4)
	set(TARGET_MCU_FREERTOS_MODULE ARM_CM4F)
	set(TARGET_MCU_CMSIS_MODULE STM32::F405RG)
elseif(TARGET_MCU)
	message(FATAL_ERROR "Unsupported target board: ${TARGET_MCU}")
else()
	message(FATAL_ERROR "TARGET_MCU must be set before attempting a build")
endif()

# Fetch STM32Cube, CMSIS and HAL
stm32_fetch_cube(${TARGET_MCU_FAMILY})

# Find CMSIS, HAL and FreeRTOS
find_package(CMSIS COMPONENTS ${CMSIS_COMP_LIST} REQUIRED)
find_package(HAL COMPONENTS ${HAL_COMP_LIST} REQUIRED)
find_package(FreeRTOS COMPONENTS ${FREERTOS_COMP_LIST} REQUIRED)

# Set target name
set(TARGET_NAME teller-obc)

add_executable(
	${TARGET_NAME}

	main.cpp

	hal/hal.cpp
	hal/stm32/led.cpp
	hal/stm32/system.cpp
	hal/stm32/uart.cpp

	tasks/blinker.cpp
	tasks/serial.cpp
)
target_include_directories(
	${TARGET_NAME} PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/hal/stm32
)
target_link_libraries(
	${TARGET_NAME} PRIVATE
	${FREERTOS_NAMESPACE}::EventGroups
	${FREERTOS_NAMESPACE}::Timers
	${FREERTOS_NAMESPACE}::Heap::4
	${FREERTOS_NAMESPACE}::${TARGET_MCU_FREERTOS_MODULE}
	${HAL_NAMESPACE}::RCC
	${HAL_NAMESPACE}::RCCEx
	${HAL_NAMESPACE}::GPIO
	${HAL_NAMESPACE}::UART
	${HAL_NAMESPACE}::CORTEX
	CMSIS::${TARGET_MCU_CMSIS_MODULE}
	STM32::NoSys
	${CMSIS_NAMESPACE}::RTOS_V2
)
target_compile_definitions(
	${TARGET_NAME} PRIVATE
	USE_CMSIS_RTOS_V2
	CMSIS_RTOS_V2_DEVICE_HEADER="${CMSIS_RTOS_V2_DEVICE_HEADER}"
)

stm32_print_size_of_target(${TARGET_NAME})
stm32_generate_binary_file(${TARGET_NAME})
stm32_generate_hex_file(${TARGET_NAME})

get_target_property(FIRMWARE_FILE ${TARGET_NAME} OUTPUT_BINARY_NAME)
set(FIRMWARE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILE})

################################################################################
# Set up simulation if the board supports simulation in qemu
################################################################################

if(TARGET_MCU STREQUAL "STM32F4")
	set(SIMULATOR_COMMAND qemu-system-arm)
	set(SIMULATOR_ARGS
		-M netduinoplus2
		-nographic
		-monitor telnet::45454,server,nowait
		-serial mon:stdio
		-kernel ${FIRMWARE_FILE}
	)
endif()

if(SIMULATOR_COMMAND)
	add_custom_target(
		simulate
		COMMAND ${SIMULATOR_COMMAND} ${SIMULATOR_ARGS}
		COMMENT "Running firmware in simulator. Use Ctrl-A X to exit..."
		VERBATIM
		USES_TERMINAL
	)
	add_dependencies(simulate ${TARGET_NAME})
endif()
