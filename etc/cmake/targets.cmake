# Macros for creating custom targets for compiling, programming and debugging
# the firmware

# set(options NONE)
# set(oneValueArgs BASE_DIRECTORY NAME)
# set(multiValueArgs EXCLUDE EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
# cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

function(add_programming_target)
    set(options "")
    set(oneValueArgs TARGET BOARD)
    set(multiValueArgs "")
    cmake_parse_arguments(OPENOCD "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    find_program(OPENOCD_PROGRAM openocd)
    if(NOT OPENOCD_PROGRAM)
        return()
    endif()

    if(NOT OPENOCD_BOARD)
        return()
    endif()

    set(TARGET_NAME "${OPENOCD_TARGET}")

	get_target_property(FIRMWARE_HEX_FILE ${TARGET_NAME} OUTPUT_HEX_NAME)
	set(FIRMWARE_HEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_HEX_FILE})

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
endfunction()

function(add_simulator_target)
    set(options "")
    set(oneValueArgs TARGET)
    set(multiValueArgs "")
    cmake_parse_arguments(SIMULATOR "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TARGET_NAME "${SIMULATOR_TARGET}")

    get_target_property(FIRMWARE_FILE ${TARGET_NAME} OUTPUT_BINARY_NAME)
    set(FIRMWARE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILE})

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
endfunction()
