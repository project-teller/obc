#include "stm32_hal.h"
#include <cmsis_os2.h>

#include "hal/system.h"
#include "hal/usb.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern USBD_HandleTypeDef hUsbDeviceFS;

static osEventFlagsId_t events;

static const uint32_t EVT_READ = 0x00000001U;
static const uint32_t EVT_WRITTEN = 0x00000002U;

static void txCompleted(void);

namespace teller::hal::usb {

bool init()
{
    events = osEventFlagsNew(nullptr);
    if (!events) {
        return false;
    }

    /* USB initialization must happen later, after we have started FreeRTOS
     * because eventually USBD_Init() will call HAL_Delay(), which will in
     * turn call vTaskDelay() */

    return true;
}

void destroy()
{
    osEventFlagsDelete(events);

    /* Not needed; we never call the destructor in STM32 */
}

bool setup()
{
    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
        return false;
    }

    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK) {
        return false;
    }

    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) {
        return false;
    }

    CDC_SetTxCpltCallback(txCompleted);

    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
        return false;
    }

    osEventFlagsClear(events, EVT_READ | EVT_WRITTEN);

    return true;
}

bool isConnected()
{
    return CDC_IsConnected();
}

bool read(std::uint8_t* data, std::uint16_t size, std::uint16_t* bytes_read)
{
    uint16_t read = 0;
    uint16_t available = 0;
    // uint32_t flags = 0;

    if (size == 0) {
        teller::hal::system::yield();
        goto exit;
    }

    if (!isConnected()) {
        return false;
    }

    do {
        available = CDC_GetRxBufferBytesAvailable_FS();
        read = available > size ? size : available;
        if (read > 0) {
            break;
        }

        /* Wait until there is something in the buffer */
        teller::hal::system::delayMsec(50);
        // flags = osEventFlagsWait(events, EVT_READ, osFlagsWaitAny, osWaitForever);
    } while (true);

    CDC_ReadRxBuffer_FS(data, read);

exit:
    if (bytes_read) {
        *bytes_read = read;
    }
    return true;
}

bool write(std::uint8_t* data, std::uint16_t size)
{
    uint32_t flags;

    if (!isConnected()) {
        return false;
    }

    if (CDC_Transmit_FS(data, size) != USBD_OK) {
        return false;
    }

    flags = osEventFlagsWait(events, EVT_WRITTEN, osFlagsWaitAll, 500);
    if (flags & osFlagsError) {
        return false;
    }

    return true;
}

}

/* ************************************************************************** */

static void txCompleted(void)
{
    osEventFlagsSet(events, EVT_WRITTEN);
}

/* ************************************************************************** */

/* IRQ handlers */

extern "C" {

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
}
