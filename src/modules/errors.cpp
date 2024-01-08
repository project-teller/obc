#include <cassert>
#include <cmsis_os2.h>

#include "hal/led.h"
#include "modules/errors.h"

#define ERROR_BIT(x) (1ULL << (x - 1))

using namespace std;
using namespace teller::hal;

/** Bitmask specifying which error codes are currently active */
static uint64_t errorCodes = 0;

/**
 * Mutex protecting the error flags to make it safe to modify it from multiple
 * tasks.
 */
static osMutexId_t errorCodeMutex;

const osMutexAttr_t error_code_mutex_attr = { "errorCodeMutex" };

void teller::errors::init()
{
    errorCodeMutex = osMutexNew(&error_code_mutex_attr);
    assert(errorCodeMutex);
}

void teller::errors::setError(error_t code, bool present)
{
    osStatus_t result;
    uint64_t oldErrorCodes;
    bool notify = false;

    result = osMutexAcquire(errorCodeMutex, osWaitForever);
    assert(result == osOK);

    oldErrorCodes = errorCodes;
    if (present) {
        errorCodes |= ERROR_BIT(code);
    } else {
        errorCodes &= ~ERROR_BIT(code);
    }
    notify = oldErrorCodes != errorCodes;

    if (notify) {
        led::set(led::ERROR, hasAnyErrors());
    }

    result = osMutexRelease(errorCodeMutex);
    assert(result == osOK);
}

void teller::errors::clearError(error_t code)
{
    setError(code, false);
}

bool teller::errors::hasError(error_t code)
{
    return errorCodes & ERROR_BIT(code);
}

bool teller::errors::hasAnyErrors()
{
    return errorCodes != 0;
}
