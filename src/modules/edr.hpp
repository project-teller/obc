#pragma once

#include <cstdlib>

#include <littlefs-cpp.h>
#include <sdlog/sdlog.h>

#include "core/telem/generic.h"
#include "hal/queue.hpp"
#include "modules/log.h"
#include "modules/storage.h"

namespace littlefs {
class Filesystem;
}

namespace teller::edr {

class ExperimentDataRecorder;
struct LogRequest;

typedef enum {
    EVENT_LOG_OPENED,
    NUM_EVENTS,
} event_t;

/**
 * @brief Type alias for event handlers that are called when a log is opened.
 */
typedef void event_callback_t(teller::telem::storage_area_t);

/**
 * Initializes the data structures required by the experiment data recorder module.
 *
 * @return whether the initialization was successful
 */
[[nodiscard]] bool init(void);

/**
 * Destroys the data structures required by the experiment data recorder module.
 */
void destroy(void);

/**
 * @brief Manages the experiment data recorder instance for the given storage area.
 *
 * This function mounts the filesystem associated with the given storage area,
 * and then associates it with the experiment data recorder and runs the recorder
 * until it is closed or until an error occurs. When an error occurs, the
 * filesystem is unmounted and the function waits until the filesystem is mounted
 * again externally, then resumes logging to the area.
 *
 * @param name  name of the storage area, used for logging
 * @param area  the storage area to manage
 */
[[noreturn]] void manage(const char* name, teller::telem::storage_area_t area);

/**
 * @brief Registers a new callback to call when a log file is opened.
 */
[[nodiscard]] bool registerCallback(event_t event, event_callback_t* callback);

/**
 * @brief Sends a new request to all attached experiment data recorder instances.
 */
void sendRequest(const LogRequest& request);

/**
 * @brief Sends a new request to a single attached experiment data recorder.
 */
void sendRequest(const LogRequest& request, teller::telem::storage_area_t area);

/**
 * @brief Unregisters an existing callback from an event.
 */
void unregisterCallback(event_t event, event_callback_t* callback);

/**
 * @brief Struct holding a single log message to be written into the experiment log.
 */
struct LogRequest {
    const sdlog_message_format_t* format;
    uint8_t message[SDLOG_MAX_MESSAGE_LENGTH];
    size_t length;
};

/** Number of log requests that can be enqueued without dropping messages */
const int QUEUE_SIZE = 16;

/**
 * @brief Class that is responsible for recording experiment data into log files
 * on a filesystem.
 *
 * An experiment data recorder may be associated to a filesystem or may be
 * in a \em detached state. In a detached state, log requests posted to the
 * recorder are ignored. When the recorder is attached to a filesystem, the
 * log requests are written into the log file on the given
 */
class ExperimentDataRecorder {

public:
    explicit ExperimentDataRecorder()
        : _queue(QUEUE_SIZE)
    {
    }
    ~ExperimentDataRecorder();

    /**
     * @brief Enqueues a new request to be logged in the experiment log.
     *
     * Returns when the request is enqueued, \em not when the request is actually
     * written to the log. No-op if the data recorder is in a detached state.
     * Blocks indefinitely if the request queue is full.
     *
     * @param request  The request to enqueue.
     */
    void record(const LogRequest& request)
    {
        if (running()) {
            _queue.send(request);
        }
    }

    /**
     * @brief Attaches the experiment data recorder to the given filesystem and runs its main loop.
     *
     * The function returns when the experiment data recorder was closed from another
     * task with its \ref close() method.
     *
     * The experiment data recorder is detached from the filesystem when this
     * function returns normally or with an exception.
     *
     * @param fs  the filesystem to attach the experiment data recorder to
     * @param area  the storage area to attach the experiment data recorder to
     */
    void run(littlefs::Filesystem* fs, teller::telem::storage_area_t area = teller::telem::STORAGE_AREA_UNKNOWN);

    /**
     * @brief Returns whether the experiment data recorder is currently running.
     */
    bool running() { return _fs != nullptr; }

    /**
     * @brief Closes the experiment data recorder.
     *
     * Requests enqueued after the data recorder is closed will be discarded
     * silently.
     */
    void stop();

private:
    littlefs::Filesystem* _fs;
    teller::hal::BlockingQueue<LogRequest> _queue;

    size_t _getLastLogIndex();
    void _run(teller::telem::storage_area_t area);
    void _updateLastLogIndex(size_t index);
};

/**
 * @brief Definition of a log record.
 *
 * This is a more convenient C++ wrapper around the \c sdlog_message_format_t
 * type from \c libsdlog. Calling the instance will encode and send a log
 * message to the log files on all storage areas.
 */
template <typename... Args>
class FormattedLogRecord {
public:
    FormattedLogRecord(
        uint8_t id, const char* msg_type, const char* names,
        const char* types, const char* units)
    {
        if (sdlog_message_format_init(&_format, id, msg_type) != SDLOG_SUCCESS) {
            throw std::runtime_error("Cannot create log record");
        }

        if (sdlog_message_format_add_columns(&_format, names, types, units) != SDLOG_SUCCESS) {
            throw std::runtime_error("Cannot create log record");
        }
    }

    ~FormattedLogRecord()
    {
        sdlog_message_format_destroy(&_format);
    }

    void write(Args... args) const
    {
        LogRequest request;

        request.format = &_format;
        if (sdlog_message_format_encode(&_format, request.message, &request.length, args...) == SDLOG_SUCCESS) {
            teller::edr::sendRequest(request);
        }
    }

private:
    sdlog_message_format_t _format;
};

}
