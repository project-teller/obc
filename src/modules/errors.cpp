#include <cmath>

#include "core/log_records.h"

#include "hal/led.h"
#include "hal/mutex.hpp"
#include "hal/system.h"

#include "modules/debug.h"
#include "modules/edr.hpp"
#include "modules/errors.h"

#define ERROR_BIT(x) (1ULL << (x - 1))

using namespace teller::debug;
using namespace teller::hal;
using namespace teller::log;

/** Bitmask specifying which error codes are currently active */
static uint64_t errorCodes = 0;

/** A single error code that we return from \ref getError() */
static teller::errors::error_t singleError = teller::errors::NO_ERROR;

/**
 * Mutex protecting the error flags to make it safe to modify it from multiple
 * tasks.
 */
static mutex errorCodeMutex;

static void logError(teller::errors::error_t code, bool present = true);
static void updateErrorLED();
static void updateSingleError();

static teller::edr::FormattedLogRecord<uint32_t, uint8_t, uint8_t>
    logRecord(LOG_RECORD_ERR, "ERR", "TimeMS,Code,Present", "IBB", "s--", "C--");

void teller::errors::init()
{
    clearAllErrors();

    // Make sure that the error LED is cleared even if the previous boot left
    // it in a state where it was lit
    updateErrorLED();
}

void teller::errors::destroy()
{
    clearAllErrors();
}

void teller::errors::setError(teller::errors::error_t code, bool present)
{
    uint64_t oldErrorCodes;
    bool notify = false;
    lock_guard<mutex> lock(errorCodeMutex);

    if (code == NO_ERROR) {
        return;
    }

    oldErrorCodes = errorCodes;
    if (present) {
        errorCodes |= ERROR_BIT(code);
        if (singleError == 0 || code < singleError) {
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
        logError(code, present);
    }
}

void teller::errors::clearError(teller::errors::error_t code)
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

bool teller::errors::hasError(teller::errors::error_t code)
{
    return errorCodes & ERROR_BIT(code);
}

bool teller::errors::hasAnyErrors()
{
    return errorCodes != 0;
}

teller::errors::error_t teller::errors::getError()
{
    return singleError;
}

void logCurrentError()
{
    logError(teller::errors::getError());
}

static void logError(teller::errors::error_t code, bool present)
{
    // This is a fragile place -- the error we are trying to log might actually
    // indicate that some queue is full or blocked so we log only in a
    // nonblocking manner
    logRecord.writeNonblocking(
        system::getTimeSinceBootMsec(),
        static_cast<uint8_t>(code),
        present ? 1 : 0);
}

static void updateErrorLED()
{
    bool hasError = teller::errors::hasAnyErrors();
    if (led::has(led::ERROR)) {
        led::set(led::ERROR, hasError);
    } else {
        /* If we do not have an error LED, change the heartbeat to indicate
         * that something is wrong */
        setBlinkPattern(hasError ? BLINK_FAST : BLINK_HEARTBEAT);
    }
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

        singleError = static_cast<teller::errors::error_t>(std::log2f(codes - (codes >> 1)) + 1);
    } else {
        singleError = teller::errors::NO_ERROR;
    }
}
