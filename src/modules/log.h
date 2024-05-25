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
 * @brief Returns a logger object corresponding to the given module.
 */
Logger* getLogger(teller::telem::module_id_t module);

/**
 * @brief Sends a log message with the given severity level and module ID.
 *
 * Shortcut for cases when you do not want to construct a logger object. You
 * should use the \ref Logger class instead where possible.
 */
bool send(
    teller::telem::module_id_t module, teller::telem::log_level_t level,
    const char* message);

/**
 * @brief Logger object that logs messages from a given module.
 */
class Logger {
public:
    Logger(teller::telem::module_id_t module = teller::telem::MODULE_ID_GENERIC);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    bool send(teller::telem::log_level_t level, const char* message) const
    {
        return teller::log::send(_module, level, message);
    }

    bool vsend(teller::telem::log_level_t level, const char* format, std::va_list args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vsnprintf(_buf, sizeof(_buf), format, args);
        return send(level, _buf);
    }

#define LOGGER_FUNC(func_name, log_level)                       \
    bool func_name(const char* format, ...)                     \
    {                                                           \
        std::va_list args;                                      \
        bool result;                                            \
                                                                \
        va_start(args, format);                                 \
        result = vsend(teller::telem::log_level, format, args); \
        va_end(args);                                           \
                                                                \
        return result;                                          \
    }

    LOGGER_FUNC(alert, LOG_LEVEL_ALERT);
    LOGGER_FUNC(critical, LOG_LEVEL_CRITICAL);
    LOGGER_FUNC(debug, LOG_LEVEL_DEBUG);
    LOGGER_FUNC(emergency, LOG_LEVEL_EMERGENCY);
    LOGGER_FUNC(error, LOG_LEVEL_ERROR);
    LOGGER_FUNC(info, LOG_LEVEL_INFO);
    LOGGER_FUNC(notice, LOG_LEVEL_NOTICE);
    LOGGER_FUNC(warning, LOG_LEVEL_WARNING);

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
