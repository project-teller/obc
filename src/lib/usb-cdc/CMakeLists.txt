add_library(
    stm32-usbd-core
    usb_device.c
    usbd_conf.c
    usbd_core.c
    usbd_ctlreq.c
    usbd_desc.c
    usbd_ioreq.c
)
target_include_directories(
    stm32-usbd-core PUBLIC include
)
target_include_directories(
    stm32-usbd-core PUBLIC ${CMAKE_SOURCE_DIR}/src/hal/stm32
)
target_link_libraries(
    stm32-usbd-core PUBLIC ${HAL_NAMESPACE}::GPIO
)

add_library(
    stm32-usbd-cdc
    usbd_cdc_if.c
    usbd_cdc.c
)
target_include_directories(stm32-usbd-cdc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32-usbd-cdc PUBLIC stm32-usbd-core)
