#include "hal/rtc.h"
#include "core/utils/time.h"

#include "config.h"
#include "stm32_hal.h"

using namespace std;

static RTC_HandleTypeDef handle;
static bool initialized = false;

// Define this if the board using the 32.768 kHz LSE clock for the RTC and
// not the 32 kHz LSI
#ifdef STM32F4
#define USES_LSE
#endif

namespace teller::hal::rtc {

bool init()
{
    bool success = false;

    /* RTC clock speed must be 1 Hz. Effective RTC clock speed is the speed
     * of its clock source divided by (AsyncPrediv + 1) * (SynchPrediv + 1).
     * AsyncPrediv must be as large as possible. Therefore, when using the
     * LSE oscillator running at 32.768 kHz, one needs AP=127 and SP=255.
     * When using the LSI at 32 kHz, one needs AP=127 and SP=249. */
    handle.Instance = RTC;
    handle.Init.HourFormat = RTC_HOURFORMAT_24;
    handle.Init.AsynchPrediv = 127;
#if defined(USES_LSE)
    handle.Init.SynchPrediv = 255;
#else
    handle.Init.SynchPrediv = 249;
#endif

    handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
#ifdef STM32H7
    handle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
#endif

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
#if defined(USES_LSE)
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
#else
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#endif
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            return;
        }

#if defined(USES_LSE) && defined(STM32H7)
        __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);
#endif

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
