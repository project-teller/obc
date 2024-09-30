set(HAL_SOURCES hal.cpp)
set(HAL_INCLUDE_DIRS "")
set(HAL_LIBRARIES lfs)

# Assemble list of source files, include directories and libraries
if(TARGET_MCU STREQUAL "POSIX")
    list(APPEND HAL_SOURCES
        posix/board.cpp
        posix/dma.cpp
        posix/gpio.cpp
        posix/imu.cpp
        posix/led.cpp
        posix/memory.cpp
        posix/rtc.cpp
        posix/spi.cpp
        posix/storage.cpp
        posix/system.cpp
        posix/uart.cpp
        posix/usb.cpp
        posix/watchdog.cpp
    )
else()
    list(APPEND HAL_SOURCES
        stm32/board.cpp
        stm32/dma.cpp
        stm32/gpio.cpp
        stm32/imu.cpp
        stm32/led.cpp
        stm32/memory.cpp
        stm32/rtc.cpp
        stm32/spi.cpp
        stm32/storage.cpp
        stm32/system.cpp
        stm32/uart.cpp
        stm32/usb.cpp
        stm32/utils.cpp
        stm32/watchdog.cpp
    )
    list(APPEND HAL_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/stm32
    )
    list(APPEND HAL_LIBRARIES
        ${FREERTOS_NAMESPACE}::EventGroups
        ${FREERTOS_NAMESPACE}::Timers
        ${FREERTOS_NAMESPACE}::Heap::4
        ${FREERTOS_NAMESPACE}::${TARGET_MCU_FREERTOS_MODULE}
        ${HAL_NAMESPACE}::CORTEX
        ${HAL_NAMESPACE}::DMA
        ${HAL_NAMESPACE}::GPIO
        ${HAL_NAMESPACE}::IWDG
        ${HAL_NAMESPACE}::LL_USB
        ${HAL_NAMESPACE}::PCD
        ${HAL_NAMESPACE}::PCDEx
        ${HAL_NAMESPACE}::PWR
        ${HAL_NAMESPACE}::PWREx
        ${HAL_NAMESPACE}::RCC
        ${HAL_NAMESPACE}::RCCEx
        ${HAL_NAMESPACE}::RTC
        ${HAL_NAMESPACE}::SPI
        ${HAL_NAMESPACE}::TIM
        ${HAL_NAMESPACE}::TIMEx
        ${HAL_NAMESPACE}::UART
        CMSIS::${TARGET_MCU_CMSIS_MODULE}
        STM32::NoSys
        ${CMSIS_NAMESPACE}::RTOS_V2
    )
    list(APPEND HAL_LIBRARIES lwrb)
    list(APPEND HAL_LIBRARIES stm32-usbd-cdc)

    if(TARGET_MCU STREQUAL "STM32H7")
        list(APPEND HAL_LIBRARIES
            ${HAL_NAMESPACE}::SPIEx
            ${HAL_NAMESPACE}::UARTEx
        )
    endif()
endif()

# Set the HSE oscillator frequency
if(TARGET_MCU STREQUAL "STM32F4")
    set(HSE_CLOCK_FREQ_MHZ 12)
else()
    set(HSE_CLOCK_FREQ_MHZ 16)
endif()

# Declare the hardware abstraction layer library
add_library(
    obc-hal
    ${HAL_SOURCES}
)
target_include_directories(
	obc-hal PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_include_directories(
	obc-hal PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_include_directories(
    obc-hal PUBLIC
    ${HAL_INCLUDE_DIRS}
)
target_link_libraries(
    obc-hal PUBLIC
    obc-core
    ${HAL_LIBRARIES}
)
target_compile_definitions(
	obc-hal PUBLIC
	USE_CMSIS_RTOS_V2
	CMSIS_RTOS_V2_DEVICE_HEADER="${CMSIS_RTOS_V2_DEVICE_HEADER}"
    HSE_VALUE=${HSE_CLOCK_FREQ_MHZ}000000U
)
