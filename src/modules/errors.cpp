#include "modules/errors.h"
#include "hal/led.h"
#include "hal/mutex.hpp"

#define ERROR_BIT(x) (1ULL << (x - 1))

using namespace teller::hal;

/** Bitmask specifying which error codes are currently active */
static uint64_t errorCodes = 0;

/**
 * Mutex protecting the error flags to make it safe to modify it from multiple
 * tasks.
 */
static mutex errorCodeMutex;

static void updateErrorLED();

void teller::errors::init()
{
    clearAllErrors();
}

void teller::errors::destroy()
{
    clearAllErrors();
}

void teller::errors::setError(error_t code, bool present)
{
    uint64_t oldErrorCodes;
    bool notify = false;
    lock_guard<mutex> lock(errorCodeMutex);

    oldErrorCodes = errorCodes;
    if (present) {
        errorCodes |= ERROR_BIT(code);
    } else {
        errorCodes &= ~ERROR_BIT(code);
    }
    notify = oldErrorCodes != errorCodes;

    if (notify) {
        updateErrorLED();
    }
}

void teller::errors::clearError(error_t code)
{
    setError(code, false);
}

void teller::errors::clearAllErrors()
{
    lock_guard<mutex> lock(errorCodeMutex);

    if (errorCodes != 0) {
        errorCodes = 0;
        updateErrorLED();
    }
}

bool teller::errors::hasError(error_t code)
{
    return errorCodes & ERROR_BIT(code);
}

bool teller::errors::hasAnyErrors()
{
    return errorCodes != 0;
}

static void updateErrorLED()
{
    led::set(led::ERROR, teller::errors::hasAnyErrors());
}
