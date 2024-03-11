#pragma once

#include "core/telem/generic.h"

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
const Logger& getLogger(teller::telem::module_id_t module);

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

    bool alert(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_ALERT, message);
    }

    bool critical(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_CRITICAL, message);
    }

    bool debug(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_DEBUG, message);
    }

    bool emergency(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_EMERGENCY, message);
    }

    bool error(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_ERROR, message);
    }

    bool info(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_INFO, message);
    }

    bool notice(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_NOTICE, message);
    }

    bool warning(const char* message) const
    {
        return send(teller::telem::LOG_LEVEL_WARNING, message);
    }

public:
    /** ID of the module associated to the logger. This cannot be private as
     * we need to initialize each logger separately in \c log.cpp , but the
     * underscore in the variable name indicates that this is de-facto private.
     */
    teller::telem::module_id_t _module;
};

}
