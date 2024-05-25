#include <cassert>
#include <cstdarg>
#include <functional>
#include <list>
#include <optional>

#include "core/utils/smart_file_handle.h"

#include "hal/storage.h"

#include "modules/log.h"
#include "modules/storage.h"

#include "modules/edr.hpp"

using namespace std;

using teller::hal::BlockingQueue;
using teller::utils::SmartFileHandle;

using namespace teller::telem;

namespace teller::edr {

bool init()
{
    return true;
}

void destroy()
{
}

/**
 * @brief Static array holding the experiment recorder instances, one for each
 * storage area.
 */
ExperimentDataRecorder recorders[NUM_STORAGE_AREAS];

/**
 * @brief List of callbacks to be called when a log is opened.
 */
std::list<event_callback_t*> callbacks;

[[noreturn]] void manage(const char* name, storage_area_t area)
{
    teller::log::Logger* log = teller::log::getLogger(MODULE_ID_EDR);

    for (;;) {
        littlefs::Filesystem* fs;
        int retval;

        fs = storage::waitUntilMounted(area);
        log->info("%s mounted", name);

        try {
            recorders[area].run(fs, area);
        } catch (...) {
            /* pass */
        }

        retval = storage::unmountStorage(area);
        if (retval) {
            log->error("%s unmount failed, code = %d", name, retval);
            storage::waitUntilUnmounted(area);
        }

        log->warning("%s unmounted, waiting for remount", name);
    }
}

bool registerCallback(event_t event, event_callback_t* callback)
{
    if (event == EVENT_LOG_OPENED) {
        callbacks.push_back(callback);
        return true;
    } else {
        return false;
    }
}

void sendRequest(const LogRequest& request)
{
    recorders[STORAGE_AREA_FLASH_MEMORY].record(request);
    recorders[STORAGE_AREA_SD_CARD].record(request);
}

void sendRequest(const LogRequest& request, storage_area_t area)
{
    assert(area > STORAGE_AREA_UNKNOWN && area < NUM_STORAGE_AREAS);
    recorders[area].record(request);
}

void unregisterCallback(event_t event, event_callback_t* callback)
{
    if (event == EVENT_LOG_OPENED) {
        callbacks.remove(callback);
    }
}

/* ************************************************************************* */
/* LogWriter implementation                                                  */
/* ************************************************************************* */

static sdlog_error_t log_writer_write(
    sdlog_ostream_t* self, const uint8_t* data, size_t length, size_t* bytes_written);
static sdlog_error_t log_writer_flush(sdlog_ostream_t* self);

static const sdlog_ostream_spec_t log_writer_spec = {
    .write = log_writer_write,
    .flush = log_writer_flush,
};

class LogWriter {
public:
    explicit LogWriter(SmartFileHandle& handle);
    ~LogWriter();

    LogWriter(const LogWriter&) = delete;
    LogWriter& operator=(const LogWriter&) = delete;

    void flush();
    void write(const LogRequest& request);

    void _flush_raw();
    size_t _write_raw(const uint8_t* data, size_t length);

private:
    SmartFileHandle& _handle;
    sdlog_ostream_t _stream;
    sdlog_writer_t _writer;
    int _init_state;
};

LogWriter::LogWriter(SmartFileHandle& handle)
    : _handle(handle)
    , _init_state(0)
{
    if (sdlog_ostream_init(&_stream, &log_writer_spec, this)) {
        throw bad_alloc();
    }
    _init_state++;

    if (sdlog_writer_init(&_writer, &_stream)) {
        throw bad_alloc();
    }
    _init_state++;
}

LogWriter::~LogWriter()
{
    if (_init_state > 1) {
        sdlog_writer_destroy(&_writer);
        _init_state--;
    }
    if (_init_state > 0) {
        sdlog_ostream_destroy(&_stream);
        _init_state--;
    }
}

void LogWriter::flush()
{
    if (sdlog_writer_flush(&_writer) != SDLOG_SUCCESS) {
        throw littlefs::Error::IO;
    }
}

void LogWriter::write(const LogRequest& request)
{
    sdlog_error_t err;

    err = sdlog_writer_write_encoded(
        &_writer, request.format, request.message, request.length);
    if (err != SDLOG_SUCCESS) {
        throw littlefs::Error::IO;
    }

    /* TODO(ntamas): we are currently flushing after every request. Check
     * whether this is necessary. Maybe we should count the total length since
     * the last flush and flush after every 4K bytes or so? */

    err = sdlog_writer_flush(&_writer);
    if (err != SDLOG_SUCCESS) {
        throw littlefs::Error::IO;
    }
}

void LogWriter::_flush_raw()
{
    _handle.sync();
}

size_t LogWriter::_write_raw(const uint8_t* data, size_t length)
{
    return _handle.write(const_cast<uint8_t*>(data), length);
}

static sdlog_error_t log_writer_write(
    sdlog_ostream_t* self, const uint8_t* data, size_t length, size_t* bytes_written)
{
    LogWriter* writer = reinterpret_cast<LogWriter*>(self->context);
    *bytes_written = writer->_write_raw(data, length);
    return SDLOG_SUCCESS;
}

static sdlog_error_t log_writer_flush(sdlog_ostream_t* self)
{
    LogWriter* writer = reinterpret_cast<LogWriter*>(self->context);
    writer->_flush_raw();
    return SDLOG_SUCCESS;
}

/* ************************************************************************* */
/* ExperimentDataRecorder implementation                                     */
/* ************************************************************************* */

const string LASTLOG_FILE("LASTLOG.TXT");

ExperimentDataRecorder::~ExperimentDataRecorder()
{
    stop();
}

void ExperimentDataRecorder::run(littlefs::Filesystem* fs, storage_area_t area)
{
    assert(!running() && !_queue.closed());

    this->_fs = fs;
    try {
        _run(area);
    } catch (const exception& e) {
        this->_fs = nullptr;
        throw;
    }
}

void ExperimentDataRecorder::stop()
{
    _queue.close();
}

/**
 * @brief Returns the index of the last log file.
 *
 * @return the index of the last log file; zero if no log file was created yet
 */
size_t ExperimentDataRecorder::_getLastLogIndex()
{
    /* Find and read LASTLOG.TXT, increase the counter by 1 */
    char buf[32];
    littlefs::Info info;

    /* Check if LASTLOG.TXT exists */
    if (this->_fs->stat(LASTLOG_FILE, &info)) {
        /* File does not exist, return 0 */
        return 0;
    }

    /* Check if LASTLOG.TXT is a regular file */
    if (info.type != LFS_TYPE_REG) {
        throw littlefs::Error::ISDIR;
    }

    /* Open LASTLOG.TXT */
    long int index;
    SmartFileHandle fd(this->_fs, this->_fs->open(LASTLOG_FILE, littlefs::OpenFlag::RDONLY));

    /* Read file contents */
    memset(buf, 0, sizeof(buf));
    fd.read(buf, sizeof(buf));
    if (sscanf(buf, "%ld", &index) != 1) {
        index = 0;
    }

    return index < 0 ? 0 : index;
}

void ExperimentDataRecorder::_run(storage_area_t area)
{
    ssize_t logIndex = _getLastLogIndex();
    char fname[32];
    LogRequest request;

    logIndex = logIndex < 0 ? 0 : (logIndex + 1);
    _updateLastLogIndex(logIndex);

    snprintf(fname, sizeof(fname), "%08ld.BIN", static_cast<long int>(logIndex));

    SmartFileHandle fd(
        this->_fs,
        this->_fs->open(
            fname,
            littlefs::OpenFlag::WRONLY | littlefs::OpenFlag::CREAT | littlefs::OpenFlag::TRUNC));
    LogWriter writer(fd);

    /* Call all callbacks to let modules print initial log records */
    if (area != STORAGE_AREA_UNKNOWN) {
        for (auto it = callbacks.begin(); it != callbacks.end(); it++) {
            (*it)(area);
        }
    }

    /* Enter the main loop and start processing requests */
    while (_queue.receive(request)) {
        writer.write(request);
    }
}

/**
 * @brief Updates the index of the last log file.
 * @return whether the operation was successful
 */
void ExperimentDataRecorder::_updateLastLogIndex(size_t index)
{
    /* Open LASTLOG.TXT */
    SmartFileHandle fd(this->_fs, this->_fs->open(LASTLOG_FILE, littlefs::OpenFlag::WRONLY | littlefs::OpenFlag::CREAT | littlefs::OpenFlag::TRUNC));
    char buf[32];
    int num_printed = snprintf(buf, sizeof(buf), "%lu", static_cast<long unsigned int>(index));
    if (num_printed < 0 || fd.write(buf, num_printed) != static_cast<size_t>(num_printed)) {
        throw littlefs::Error::IO;
    }
}

}
