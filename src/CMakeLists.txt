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

# Create configuration file
configure_file(config.h.in config.h)

# Figure out which components to link to
set(HAL_COMP_LIST RCC GPIO UART CORTEX DMA IWDG)
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

# Specify source files, include paths and libraries to link to
add_executable(
	${TARGET_NAME}

	main.cpp

	hal/hal.cpp
	hal/stm32/led.cpp
	hal/stm32/system.cpp
	hal/stm32/uart.cpp
	hal/stm32/utils.cpp
	hal/stm32/watchdog.cpp

	modules/errors.cpp
	modules/telem.cpp

	tasks/blinker.cpp
	tasks/serial.cpp
	tasks/supervisor.cpp
	tasks/telem.cpp

	utils/crc.c
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
	${HAL_NAMESPACE}::DMA
	${HAL_NAMESPACE}::IWDG
	${HAL_NAMESPACE}::UART
	${HAL_NAMESPACE}::UARTEx
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

# Do not use C++ exceptions and RTTI, but ask for all compiler warnings
set_target_properties(
	${TARGET_NAME}
	PROPERTIES
	CXX_RTTI OFF
	CXX_EXCEPTIONS OFF
	CXX_WARNINGS ALL
	CXX_WARNINGS_AS_ERRORS ON
)

stm32_print_size_of_target(${TARGET_NAME})
stm32_generate_binary_file(${TARGET_NAME})
stm32_generate_hex_file(${TARGET_NAME})

get_target_property(FIRMWARE_FILE ${TARGET_NAME} OUTPUT_BINARY_NAME)
set(FIRMWARE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILE})

get_target_property(FIRMWARE_HEX_FILE ${TARGET_NAME} OUTPUT_HEX_NAME)
set(FIRMWARE_HEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_HEX_FILE})

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

################################################################################
# Set up programming target if we know how to program the board with OpenOCD
################################################################################

if(OPENOCD_PROGRAM AND OPENOCD_BOARD)
	set(OPENOCD_ARGS
		-f board/${OPENOCD_BOARD}.cfg
		-c "program ${FIRMWARE_HEX_FILE} verify reset exit"
	)

    get_target_property(TARGET_OUTPUT_NAME ${TARGET_NAME} OUTPUT_NAME)
    if(TARGET_OUTPUT_NAME)
        set(TARGET_OUTPUT_NAME "${TARGET_OUTPUT_NAME}.elf")
    else()
        set(TARGET_OUTPUT_NAME "${TARGET_NAME}.elf")
    endif()
	set(DEBUG_ARGS
		board/${OPENOCD_BOARD}.cfg
		${TARGET_OUTPUT_NAME}
	)

	add_custom_target(
		upload
		COMMAND ${OPENOCD_PROGRAM} ${OPENOCD_ARGS}
		COMMENT "Uploading firmware to board..."
		VERBATIM
		USES_TERMINAL
	)
	add_dependencies(upload ${TARGET_NAME})

	add_custom_target(
		debug
		COMMAND ${CMAKE_SOURCE_DIR}/etc/scripts/debug.sh ${DEBUG_ARGS}
		COMMENT "Running debugger for firmware to board..."
		VERBATIM
		USES_TERMINAL
	)
	add_dependencies(debug upload)
endif()
