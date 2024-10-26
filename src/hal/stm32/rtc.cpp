#include "hal/rtc.h"
#include "core/utils/time.h"
#include "hal/system.h"

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
    broken_down_time_t bt;
    uint64_t result;

    /* The STM32 documentation says that HAL_RTC_GetDate() must be called
     * after HAL_RTC_GetTime() */

    if (HAL_RTC_GetTime(&handle, &time, RTC_FORMAT_BIN) != HAL_OK) {
        return 0;
    }

    if (HAL_RTC_GetDate(&handle, &date, RTC_FORMAT_BIN) != HAL_OK) {
        return 0;
    }

    bt.year = date.Year + 2000;
    bt.month = date.Month;
    bt.day = date.Date;
    bt.hour = time.Hours;
    bt.minute = time.Minutes;
    bt.second = time.Seconds;
    bt.millisecond = 1000 * time.SubSeconds / (static_cast<float>(time.SecondFraction) + 1);

    if (!utcTimeToMsec(&bt, &result)) {
        return 0;
    }

    return result;
}

bool setTimeMsec(uint64_t timestamp)
{
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    broken_down_time_t bt;
    uint64_t remainder;

    if (!utcMsecToTime(timestamp, &bt)) {
        return false;
    }

    if (bt.year < 2000) {
        return false;
    }

    /* HAL_RTC_SetTime can only set the clock to whole seconds. So, if the
     * requested timestamp is not a whole second, we need to wait. */
    remainder = timestamp % 1000;
    if (remainder > 0) {
        remainder = 1000 - remainder;
        timestamp += remainder;
        teller::hal::system::delayMsec(remainder);
    }

    if (!utcMsecToTime(timestamp, &bt)) {
        return false;
    }

    date.Year = bt.year - 2000;
    date.Month = bt.month;
    date.Date = bt.day;
    date.WeekDay = utcTimeToDayOfWeek(&bt);
    if (date.WeekDay == 0) {
        date.WeekDay = 7; /* STM32 HAL day indices go from 1 (Monday) to 7 (Sunday) */
    }
    time.Hours = bt.hour;
    time.Minutes = bt.minute;
    time.Seconds = bt.second;

    /* HAL_RTC_SetDate needs to be called first, followed by HAL_RTC_SetTime.
     * This is not clear from the docs but it can be found in the STM32
     * forum. */
    if (HAL_RTC_SetDate(&handle, &date, RTC_FORMAT_BIN) != HAL_OK) {
        return false;
    }

    if (HAL_RTC_SetTime(&handle, &time, RTC_FORMAT_BIN) != HAL_OK) {
        return false;
    }

    return true;
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
