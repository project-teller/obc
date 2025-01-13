#include <cassert>
#include <cstdarg>
#include <functional>
#include <list>
#include <optional>

#include "core/utils/smart_file_handle.h"
#include "hal/memory.h"
#include "hal/system.h"
#include "modules/edr.hpp"
#include "modules/log.h"
#include "modules/storage.h"

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
static ExperimentDataRecorder recorders[NUM_STORAGE_AREAS];

/**
 * @brief Sentinel element used to stop a running experiment data recorder.
 */
static const LogRequest stopRecorderRequest = { 0 };

/**
 * @brief Time between consecutive sync operations, in milliseconds.
 */
#define SYNC_INTERVAL_MS 30000

/**
 * @brief List of callbacks to be called when a log is opened.
 */
static std::list<event_callback_t*> callbacks;

[[noreturn]] void manage(storage_area_t area)
{
    teller::log::Logger* log = teller::log::getLogger(MODULE_ID_EDR);
    const char* name = getStorageAreaName(area);

    /* Apparently a small delay is needed at boot to avoid problems with
     * the initialization */
    teller::hal::system::delayMsec(1000);

    for (;;) {
        littlefs::Filesystem* fs;
        int retval;

        fs = storage::waitUntilMounted(area);
        log->info("%s: mounted", name);

        auto maybeError = recorders[area].run(fs, area);
        if (maybeError.has_value()) {
            log->error(
                "%s: IO error, code = %d", name,
                teller::storage::convertLittleFSErrorCode(*maybeError));
        }

        retval = storage::unmountStorage(area);
        if (retval) {
            log->error("%s: unmount failed, code = %d", name, retval);
        }

        storage::waitUntilUnmounted(area);
        log->warning("%s: unmounted, waiting for remount", name);
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

bool requestStopAndUnmount(teller::telem::storage_area_t area)
{
    if (recorders[area].running()) {
        sendRequest(stopRecorderRequest, area);
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

void sendRequestNonblocking(const LogRequest& request)
{
    recorders[STORAGE_AREA_FLASH_MEMORY].recordNonblocking(request);
    recorders[STORAGE_AREA_SD_CARD].recordNonblocking(request);
}

void sendRequestNonblocking(const LogRequest& request, storage_area_t area)
{
    assert(area > STORAGE_AREA_UNKNOWN && area < NUM_STORAGE_AREAS);
    recorders[area].recordNonblocking(request);
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

    [[nodiscard]] std::optional<littlefs::Error> flush();
    [[nodiscard]] std::optional<littlefs::Error> write(const LogRequest& request);

    [[nodiscard]] std::optional<littlefs::Error> _flush_raw();
    [[nodiscard]] std::variant<littlefs::Error, size_t> _write_raw(const uint8_t* data, size_t length);

private:
    SmartFileHandle& _handle;
    sdlog_ostream_t _stream;
    sdlog_writer_t _writer;
    int _init_state;
    uint32_t _last_sync_at;
};

LogWriter::LogWriter(SmartFileHandle& handle)
    : _handle(handle)
    , _init_state(0)
    , _last_sync_at(0)
{
    sdlog_error_t err;

    err = sdlog_ostream_init(&_stream, &log_writer_spec, this);
    assert(err == SDLOG_SUCCESS);
    _init_state++;

    err = sdlog_writer_init(&_writer, &_stream);
    assert(err == SDLOG_SUCCESS);

    _init_state++;

    (void)(err); /* prevent a gcc unused variable warning in release builds */
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

std::optional<littlefs::Error> LogWriter::flush()
{
    if (sdlog_writer_flush(&_writer) != SDLOG_SUCCESS) {
        return littlefs::Error::IO;
    } else {
        return {};
    }
}

std::optional<littlefs::Error> LogWriter::write(const LogRequest& request)
{
    sdlog_error_t err;

    err = sdlog_writer_write_encoded(
        &_writer, request.format, request.message, request.length);
    if (err != SDLOG_SUCCESS) {
        return littlefs::Error::IO;
    }

    /* TODO(ntamas): we are currently flushing after every request. Check
     * whether this is necessary. Maybe we should count the total length since
     * the last flush and flush after every 4K bytes or so? */

    err = sdlog_writer_flush(&_writer);
    if (err != SDLOG_SUCCESS) {
        return littlefs::Error::IO;
    }

    return {};
}

std::optional<littlefs::Error> LogWriter::_flush_raw()
{
    uint32_t now = teller::hal::system::getTimeSinceBootMsec();
    if (_last_sync_at + SYNC_INTERVAL_MS < now) {
        _last_sync_at = now;
        return _handle.sync();
    } else {
        return {};
    }
}

std::variant<littlefs::Error, size_t> LogWriter::_write_raw(const uint8_t* data, size_t length)
{
    return _handle.write(const_cast<uint8_t*>(data), length);
}

static sdlog_error_t log_writer_write(
    sdlog_ostream_t* self, const uint8_t* data, size_t length, size_t* bytes_written)
{
    LogWriter* writer = reinterpret_cast<LogWriter*>(self->context);
    auto result = writer->_write_raw(data, length);
    if (std::holds_alternative<littlefs::Error>(result)) {
        return SDLOG_EIO;
    } else {
        *bytes_written = std::get<size_t>(result);
        return SDLOG_SUCCESS;
    }
}

static sdlog_error_t log_writer_flush(sdlog_ostream_t* self)
{
    LogWriter* writer = reinterpret_cast<LogWriter*>(self->context);
    auto result = writer->_flush_raw();
    return result ? SDLOG_EIO : SDLOG_SUCCESS;
}

/* ************************************************************************* */
/* ExperimentDataRecorder implementation                                     */
/* ************************************************************************* */

const string LASTLOG_FILE("LASTLOG.TXT");

ExperimentDataRecorder::~ExperimentDataRecorder()
{
    stop();
}

std::optional<littlefs::Error> ExperimentDataRecorder::run(littlefs::Filesystem* fs, storage_area_t area)
{
    void* buffer = nullptr;

    assert(!running() && !_queue.closed());

    buffer = teller::hal::memory::malloc(fs->cache_size());
    assert(buffer != nullptr);

    this->_fs = fs;
    _queue.clear();

    this->_file_config = make_unique<littlefs::FileConfig>(buffer);
    assert(this->_file_config != nullptr);

    auto result = _run(area);

    this->_file_config = nullptr;
    this->_fs = nullptr;
    teller::hal::memory::free(buffer);

    return result;
}

void ExperimentDataRecorder::stop()
{
    _queue.close();
}

/**
 * @brief Returns the index of the last log file.
 *
 * @return the index of the last log file; zero if no log file was created yet,
 * or a LittleFS error code.
 */
std::variant<littlefs::Error, size_t> ExperimentDataRecorder::_getLastLogIndex()
{
    /* Find and read LASTLOG.TXT, increase the counter by 1 */
    char buf[32];
    littlefs::Info info;

    /* Check if LASTLOG.TXT exists */
    if (this->_fs->stat(LASTLOG_FILE, &info)) {
        /* File does not exist, return 0 */
        return static_cast<size_t>(0);
    }

    /* Check if LASTLOG.TXT is a regular file */
    if (info.type != LFS_TYPE_REG) {
        return littlefs::Error::ISDIR;
    }

    /* Open LASTLOG.TXT */
    long int index;
    auto maybe_handle = this->_fs->opencfg(LASTLOG_FILE, littlefs::OpenFlag::RDONLY, *this->_file_config);
    if (std::holds_alternative<littlefs::Error>(maybe_handle)) {
        return std::get<littlefs::Error>(maybe_handle);
    }

    SmartFileHandle fd(this->_fs, std::get<littlefs::FileHandle>(maybe_handle));

    /* Read file contents */
    memset(buf, 0, sizeof(buf));
    fd.read(buf, sizeof(buf));
    if (sscanf(buf, "%ld", &index) != 1) {
        index = 0;
    }

    return static_cast<size_t>(index < 0 ? 0 : index);
}

#define IS_END_OF_QUEUE(request) (request.format == nullptr)
#define RETURN_IF_ERROR(x)   \
    {                        \
        if (x.has_value()) { \
            return x;        \
        }                    \
    }
#define RETURN_IF_ERROR_VARIANT(x)                        \
    {                                                     \
        if (std::holds_alternative<littlefs::Error>(x)) { \
            return std::get<littlefs::Error>(x);          \
        }                                                 \
    }

std::optional<littlefs::Error> ExperimentDataRecorder::_run(storage_area_t area)
{
    size_t logIndex;
    char fname[32];
    LogRequest request;

    assert(this->_file_config);

    auto maybeLogIndex = _getLastLogIndex();
    RETURN_IF_ERROR_VARIANT(maybeLogIndex);

    logIndex = std::get<size_t>(maybeLogIndex) + 1;
    auto result = _updateLastLogIndex(logIndex);
    RETURN_IF_ERROR(result);

    snprintf(fname, sizeof(fname), "%08ld.BIN", static_cast<long int>(logIndex));

    auto maybeFileHandle = this->_fs->opencfg(
        fname,
        littlefs::OpenFlag::WRONLY | littlefs::OpenFlag::CREAT | littlefs::OpenFlag::TRUNC,
        *_file_config);
    RETURN_IF_ERROR_VARIANT(maybeFileHandle);

    SmartFileHandle fd(this->_fs, std::get<littlefs::FileHandle>(maybeFileHandle));
    LogWriter writer(fd);

    /* Enter the "starting" state so callbacks can write their log records */
    _state = STATE_STARTING;

    /* Call all callbacks to let modules print initial log records. We have to
     * be careful here; the callback should not create too many messages that
     * would block the _queue before we start draining them.
     *
     * If this becomes a problem, we should drain the queue after every callback.
     */
    if (area != STORAGE_AREA_UNKNOWN) {
        for (auto it = callbacks.begin(); it != callbacks.end(); it++) {
            (*it)(area);
        }
    }

    /* Enter the main loop and start processing requests */
    std::optional<littlefs::Error> maybeError = std::nullopt;

    _state = STATE_RUNNING;
    while (_queue.receive(request) && !IS_END_OF_QUEUE(request) && !maybeError) {
        maybeError = writer.write(request);
    }
    _state = STATE_DISABLED;

    return maybeError;
}

#undef IS_END_OF_QUEUE

/**
 * @brief Updates the index of the last log file.
 * @return whether the operation was successful
 */
std::optional<littlefs::Error> ExperimentDataRecorder::_updateLastLogIndex(size_t index)
{
    /* Open LASTLOG.TXT */
    auto maybeFileHandle = this->_fs->opencfg(
        LASTLOG_FILE,
        littlefs::OpenFlag::WRONLY | littlefs::OpenFlag::CREAT | littlefs::OpenFlag::TRUNC,
        *_file_config);
    RETURN_IF_ERROR_VARIANT(maybeFileHandle);

    SmartFileHandle fd(this->_fs, std::get<littlefs::FileHandle>(maybeFileHandle));
    char buf[32];
    int num_printed = snprintf(buf, sizeof(buf), "%lu", static_cast<long unsigned int>(index));
    if (num_printed < 0) {
        return littlefs::Error::IO;
    }

    auto result = fd.write(buf, num_printed);
    RETURN_IF_ERROR_VARIANT(result);

    if (std::get<size_t>(result) != static_cast<size_t>(num_printed)) {
        return littlefs::Error::IO;
    }

    return {};
}
}
