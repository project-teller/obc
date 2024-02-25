#include "hal/rtc.h"
#include "core/utils/time.h"

#include "stm32_hal.h"

using namespace std;

static RTC_HandleTypeDef handle;
static bool initialized = false;

namespace teller::hal::rtc {

bool init()
{
    bool success = false;

    handle.Instance = RTC;
    handle.Init.HourFormat = RTC_HOURFORMAT_24;
    handle.Init.AsynchPrediv = 127;
    handle.Init.SynchPrediv = 255;
    handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    handle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;

    if (HAL_RTC_Init(&handle) != HAL_OK) {
        goto cleanup;
    }

    /* HAL_RTC_MspInit sets state->initialized to true. If this is still false
     * at this point, it means that the initialization was not successful */
    if (!initialized) {
        goto cleanup;
    }

    success = true;

cleanup:
    return success;
}

void destroy()
{
    if (initialized) {
        HAL_RTC_DeInit(&handle);
        initialized = false;
    }
}

uint64_t getTimeMsec()
{
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;

    /* The STM32 documentation says that HAL_RTC_GetDate() must be called
     * after HAL_RTC_GetTime() */

    if (HAL_RTC_GetTime(&handle, &time, RTC_FORMAT_BIN) != HAL_OK) {
        return 0;
    }

    if (HAL_RTC_GetDate(&handle, &date, RTC_FORMAT_BIN) != HAL_OK) {
        return 0;
    }

    return utcTimeToMsec(
        date.Year + 2000, date.Month, date.Date,
        time.Hours, time.Minutes, time.Seconds,
        1000 * time.SubSeconds / (static_cast<float>(time.SecondFraction) + 1));
}

bool setTimeMsec(uint64_t timestamp)
{
    return false;
}

}

/* ************************************************************************** */

/* Weakly linked function that is called by the STM32 HAL when an RTC is
 * initialized
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    if (hrtc->Instance == RTC) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }

        __HAL_RCC_RTC_ENABLE();

        initialized = true;
    }
}

/* Weakly linked function that is called by the STM32 HAL when an RTC is
 * deinitialized
 */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
{
    if (hrtc->Instance == RTC) {
        __HAL_RCC_RTC_DISABLE();
    }
}
