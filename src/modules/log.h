#pragma once

#include <cstdarg>
#include <cstdio>

#include "core/telem/generic.h"
#include "hal/mutex.hpp"

namespace teller::log {

class Logger;

/**
 * @brief Initializes the logging module of the experiment.
 */
[[nodiscard]] bool init(void);

/**
 * @brief Destroys the logging module of the experiment.
 */
void destroy(void);

/**
 * @brief Runs a single iteration of the main loop of the logging module.
 *
 * This function must be called periodically from a logging task. The function
 * must produce telemetry messages and add new records to the onboard logs
 * according to a predefined log schedule, and then return. It is the duty
 * of the caller to wait before calling this function again.
 */
void runSingleIteration(void);

/**
 * @brief Returns a logger object corresponding to the given module.
 *
 * Loggers are thread-safe; you can call them from multiple tasks, but the
 * logger object may block until the log message was sent to the telemetry
 * module.
 */
Logger* getLogger(teller::telem::module_id_t module);

/**
 * @brief Sends a log message with the given severity level and module ID.
 *
 * Shortcut for cases when you do not want to construct a logger object. You
 * should use the \ref Logger class instead where possible.
 */
bool sendToTelemetry(
    teller::telem::module_id_t module, teller::telem::log_level_t level,
    const char* message, uint32_t timeout = teller::telem::DEFAULT_TIMEOUT);

/**
 * @brief Logger object that logs messages from a given module.
 */
class Logger {
public:
    Logger(teller::telem::module_id_t module = teller::telem::MODULE_ID_GENERIC);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    bool send(
        teller::telem::log_level_t level, const char* message,
        std::uint32_t timeout) const
    {
        return teller::log::sendToTelemetry(_module, level, message, timeout);
    }

    bool vsend(
        teller::telem::log_level_t level, const char* format, std::va_list args,
        std::uint32_t timeout)
    {
        teller::hal::lock_guard<teller::hal::mutex> lock(_mutex);
        std::vsnprintf(_buf, sizeof(_buf), format, args);
        return send(level, _buf, timeout);
    }

#define LOGGER_FUNC(func_name, log_level, timeout)                       \
    bool func_name(const char* format, ...)                              \
    {                                                                    \
        std::va_list args;                                               \
        bool result;                                                     \
                                                                         \
        va_start(args, format);                                          \
        result = vsend(teller::telem::log_level, format, args, timeout); \
        va_end(args);                                                    \
                                                                         \
        return result;                                                   \
    }

    LOGGER_FUNC(alert, LOG_LEVEL_ALERT, teller::telem::DEFAULT_TIMEOUT);
    LOGGER_FUNC(critical, LOG_LEVEL_CRITICAL, teller::telem::DEFAULT_TIMEOUT);
    LOGGER_FUNC(debug, LOG_LEVEL_DEBUG, teller::telem::DEFAULT_TIMEOUT);
    LOGGER_FUNC(emergency, LOG_LEVEL_EMERGENCY, teller::telem::DEFAULT_TIMEOUT);
    LOGGER_FUNC(error, LOG_LEVEL_ERROR, teller::telem::DEFAULT_TIMEOUT);
    LOGGER_FUNC(info, LOG_LEVEL_INFO, teller::telem::DEFAULT_TIMEOUT);
    LOGGER_FUNC(notice, LOG_LEVEL_NOTICE, teller::telem::DEFAULT_TIMEOUT);
    LOGGER_FUNC(warning, LOG_LEVEL_WARNING, teller::telem::DEFAULT_TIMEOUT);

    LOGGER_FUNC(alert_nowait, LOG_LEVEL_ALERT, 0);
    LOGGER_FUNC(critical_nowait, LOG_LEVEL_CRITICAL, 0);
    LOGGER_FUNC(debug_nowait, LOG_LEVEL_DEBUG, 0);
    LOGGER_FUNC(emergency_nowait, LOG_LEVEL_EMERGENCY, 0);
    LOGGER_FUNC(error_nowait, LOG_LEVEL_ERROR, 0);
    LOGGER_FUNC(info_nowait, LOG_LEVEL_INFO, 0);
    LOGGER_FUNC(notice_nowait, LOG_LEVEL_NOTICE, 0);
    LOGGER_FUNC(warning_nowait, LOG_LEVEL_WARNING, 0);

public:
    /** ID of the module associated to the logger. This cannot be private as
     * we need to initialize each logger separately in \c log.cpp , but the
     * underscore in the variable name indicates that this is de-facto private.
     */
    teller::telem::module_id_t _module;

    /** Internal buffer that the logger will use to format messages */
    char _buf[teller::telem::MAX_PAYLOAD_LENGTH];

    /** Mutex to control access to the internal buffer */
    teller::hal::mutex _mutex;
};

}
