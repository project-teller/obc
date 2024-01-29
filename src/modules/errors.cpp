#include <cmath>

#include "hal/led.h"
#include "hal/mutex.hpp"
#include "modules/errors.h"

#define ERROR_BIT(x) (1ULL << (x - 1))

using namespace teller::errors;
using namespace teller::hal;

/** Bitmask specifying which error codes are currently active */
static uint64_t errorCodes = 0;

/** A single error code that we return from \ref getError() */
static error_t singleError = NO_ERROR;

/**
 * Mutex protecting the error flags to make it safe to modify it from multiple
 * tasks.
 */
static mutex errorCodeMutex;

static void updateErrorLED();
static void updateSingleError();

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
        if (code > singleError) {
            singleError = code;
        }
    } else {
        errorCodes &= ~ERROR_BIT(code);
        if (singleError == code) {
            updateSingleError();
        }
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
        updateSingleError();
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

error_t teller::errors::getError()
{
    return singleError;
}

static void updateErrorLED()
{
    led::set(led::ERROR, teller::errors::hasAnyErrors());
}

static void updateSingleError()
{
    uint64_t codes = errorCodes;

    if (codes) {
        codes |= (codes >> 1);
        codes |= (codes >> 2);
        codes |= (codes >> 4);
        codes |= (codes >> 8);
        codes |= (codes >> 16);
        codes |= (codes >> 32);

        singleError = static_cast<error_t>(std::log2f(codes - (codes >> 1)) + 1);
    } else {
        singleError = NO_ERROR;
    }
}
