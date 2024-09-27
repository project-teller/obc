#include "stm32_hal.h"

#include "hal/usb.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
static USBD_HandleTypeDef usb_device_handle;

namespace teller::hal::usb {

bool init()
{
    /* USB initialization must happen later, after we have started FreeRTOS
     * because eventually USBD_Init() will call HAL_Delay(), which will in
     * turn call vTaskDelay() */
    return true;
}

void destroy()
{
    /* Not needed; we never call the destructor in STM32 */
}

bool setup()
{
    if (USBD_Init(&usb_device_handle, &FS_Desc, DEVICE_FS) != USBD_OK) {
        return false;
    }

    if (USBD_RegisterClass(&usb_device_handle, &USBD_CDC) != USBD_OK) {
        return false;
    }

    if (USBD_CDC_RegisterInterface(&usb_device_handle, &USBD_Interface_fops_FS) != USBD_OK) {
        return false;
    }

    if (USBD_Start(&usb_device_handle) != USBD_OK) {
        return false;
    }

    return true;
}

}

/* ************************************************************************** */

/* IRQ handlers */

extern "C" {

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
}
