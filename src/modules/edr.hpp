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
 * @param area  the storage area to manage
 */
[[noreturn]] void manage(teller::telem::storage_area_t area);

/**
 * @brief Registers a new callback to call when a log file is opened.
 */
[[nodiscard]] bool registerCallback(event_t event, event_callback_t* callback);

/**
 * @brief Requests the experiment data recorder corresponding to the given area
 * to be stopped.
 *
 * This function adds a sentinel LogRequest to the end of the request queue of
 * the experiment data recorder instance corresponding to the given storage
 * area. When the sentinel request is reached, the request loop of the EDR will
 * exit and the EDR task will in turn unmount the storage area.
 *
 * @param area  the storage area to unmount
 * @return whether the unmount request was posted successfully; false if the
 *         area was not mounted when the function was called
 */
bool requestStopAndUnmount(teller::telem::storage_area_t area);

/**
 * @brief Sends a new request to all attached experiment data recorder instances.
 */
void sendRequest(const LogRequest& request);

/**
 * @brief Sends a new request to a single attached experiment data recorder.
 */
void sendRequest(const LogRequest& request, teller::telem::storage_area_t area);

/**
 * @brief Sends a new request to all attached experiment data recorder instances in a non-blocking manner.
 */
void sendRequestNonblocking(const LogRequest& request);

/**
 * @brief Sends a new request to a single attached experiment data recorder in a non-blocking manner.
 */
void sendRequestNonblocking(const LogRequest& request, teller::telem::storage_area_t area);

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

/** Number of milliseconds to wait for log requests to be written to the log */
const int LOG_TIMEOUT_MS = 100;

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

private:
    enum State {
        STATE_DISABLED = 0,
        STATE_STARTING = 1,
        STATE_RUNNING = 2
    };

public:
    explicit ExperimentDataRecorder()
        : _state(STATE_DISABLED)
        , _queue(QUEUE_SIZE)
        , _timeout(LOG_TIMEOUT_MS)
        , _dropped(0)
    {
    }
    ~ExperimentDataRecorder();

    /**
     * @brief Returns the number of dropped log records and resets the counter.
     */
    uint32_t getAndClearDroppedCounter()
    {
        uint32_t dropped = _dropped;
        _dropped = 0;
        return dropped;
    }

    /**
     * @brief Enqueues a new request to be logged in the experiment log.
     *
     * Returns when the request is enqueued, \em not when the request is actually
     * written to the log. No-op if the data recorder is in a detached state.
     * Blocks with a timeout if the request queue is full, and then increases a
     * counter if the request was dropped.
     *
     * @param request  The request to enqueue.
     */
    void record(const LogRequest& request)
    {
        if (running() || starting()) {
            if (!_queue.send_with_timeout(request, _timeout)) {
                _dropped++;
            }
        }
    }

    /**
     * @brief Enqueues a new request to be logged in the experiment log, without blocking.
     *
     * Returns when the request is enqueued, \em not when the request is actually
     * written to the log. No-op if the data recorder is in a detached state.
     * Drops the request immediately if there is no space in the request queue.
     *
     * @param request  The request to enqueue.
     */
    void recordNonblocking(const LogRequest& request)
    {
        if (running() || starting()) {
            if (!_queue.send_or_drop(request)) {
                _dropped++;
            }
        }
    }

    /**
     * @brief Attaches the experiment data recorder to the given filesystem and runs its main loop.
     *
     * The function returns when the experiment data recorder was closed from another
     * task with its \ref close() method or when an error occurs.
     *
     * The experiment data recorder is detached from the filesystem when this
     * function returns normally or with an exception.
     *
     * @param fs  the filesystem to attach the experiment data recorder to
     * @param area  the storage area to attach the experiment data recorder to
     * @return  an optional error code
     */
    [[nodiscard]] std::optional<littlefs::Error> run(
        littlefs::Filesystem* fs,
        teller::telem::storage_area_t area = teller::telem::STORAGE_AREA_UNKNOWN);

    /**
     * @brief Returns whether the experiment data recorder is currently running.
     */
    bool running() { return _state == STATE_RUNNING; }

    /**
     * @brief Returns whether the experiment data recorder is currently starting.
     */
    bool starting() { return _state == STATE_STARTING; }

    /**
     * @brief Closes the experiment data recorder.
     *
     * Requests enqueued after the data recorder is closed will be discarded
     * silently.
     */
    void stop();

private:
    littlefs::Filesystem* _fs;
    std::unique_ptr<littlefs::FileConfig> _file_config;
    State _state;
    teller::hal::BlockingQueue<LogRequest> _queue;
    uint32_t _timeout;
    uint32_t _dropped;

    [[nodiscard]] std::variant<littlefs::Error, size_t> _getLastLogIndex();
    [[nodiscard]] std::optional<littlefs::Error> _run(teller::telem::storage_area_t area);
    [[nodiscard]] std::optional<littlefs::Error> _updateLastLogIndex(size_t index);
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
        const char* types, const char* units, const char* multipliers)
    {
        sdlog_error_t err;

        err = sdlog_message_format_init(&_format, id, msg_type);
        assert(err == SDLOG_SUCCESS);

        err = sdlog_message_format_add_columns(&_format, names, types, units);
        assert(err == SDLOG_SUCCESS);

        (void)(err); /* prevent a gcc unused variable warning in release builds */
    }

    ~FormattedLogRecord()
    {
        sdlog_message_format_destroy(&_format);
    }

    void write(Args... args) const
    {
        std::unique_ptr<LogRequest> request = std::make_unique<LogRequest>();

        request->format = &_format;
        if (sdlog_message_format_encode(&_format, request->message, &request->length, args...) == SDLOG_SUCCESS) {
            teller::edr::sendRequest(*request);
        }
    }

    void writeNonblocking(Args... args) const
    {
        std::unique_ptr<LogRequest> request = std::make_unique<LogRequest>();

        request->format = &_format;
        if (sdlog_message_format_encode(&_format, request->message, &request->length, args...) == SDLOG_SUCCESS) {
            teller::edr::sendRequestNonblocking(*request);
        }
    }

private:
    sdlog_message_format_t _format;
};

}
