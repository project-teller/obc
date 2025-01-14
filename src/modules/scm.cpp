#include <algorithm>
#include <minmea.h>

#include "core/log_records.h"
#include "core/nmea/parser.h"
#include "core/telem/scm.h"
#include "core/utils/histogram.h"
#include "core/utils/varuint.h"

#include "hal/system.h"
#include "hal/uart.h"

#include "modules/edr.hpp"
#include "modules/log.h"
#include "modules/scm.h"
#include "modules/telem.h"

// Uncomment the next line to simulate the SCM instead of reading from the SCM UART
// #define SIMULATE_SCM

using namespace teller::hal;
using namespace teller::log;
using namespace teller::math;
using namespace teller::telem;

static teller::edr::FormattedLogRecord<
    uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t*>
    logRecord(
        LOG_RECORD_SCM, "SCM",
        "TimeMS,I,MaxFrag,Frag,Length,Histogram",
        "IBBBBZ", "s-----", "C-----");

#define NUM_SCINTILLATORS 3
#define HISTOGRAM_SIZE 256
#define FRAGMENT_SIZE teller::telem::frames::MAX_SCM_FRAME_FRAGMENT_LENGTH

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

static Logger* logger;
static uint32_t lastMessageStartedAt;
static frames::scm_data_t scmFrame;
static varuint_decoder_t varuintDecoder;

static uint8_t scintillatorIndex;
static uint16_t histogram[HISTOGRAM_SIZE];
static uint16_t histogramIndex;
static uint8_t packedHistogram[HISTOGRAM_SIZE * 2];
static uint8_t* packedHistogramEnd;

static void logSCMMeasurement(void);
static void sendSCMMeasurement(uint8_t* payload);
static bool updateStatus(void);

static void addNonzeroToHistogram(uint32_t value);
static void addZerosToHistogram(uint32_t count);
static bool packHistogram(void);
static void startHistogramForScintillator(uint8_t index);

namespace teller::scm {

bool init()
{
    status = SUBSYSTEM_STATUS_CRITICAL;
    logger = getLogger(MODULE_ID_SCM);

    lastMessageStartedAt = 0;

    varuint_decoder_init(&varuintDecoder);

    startHistogramForScintillator(0);

    return logger != nullptr;
}

void destroy()
{
    varuint_decoder_destroy(&varuintDecoder);
    logger = nullptr;
    status = SUBSYSTEM_STATUS_CRITICAL;
}

subsystem_status_t getSubsystemStatus()
{
    return status;
}

bool setup()
{
    lastMessageStartedAt = 0;
#ifdef SIMULATE_SCM
    status = SUBSYSTEM_STATUS_OK;
#else
    status = uart::isConnected(uart::SCM) ? SUBSYSTEM_STATUS_OK : SUBSYSTEM_STATUS_CRITICAL;
#endif
    return status == SUBSYSTEM_STATUS_OK;
}

bool update(uint8_t* payload, bool& updated)
{
#ifdef SIMULATE_SCM
    /* Pretend that the SCM is connected and send a dummy measurement 10 times
     * per second */
    system::sleepUntilMsec(lastMessageStartedAt + 100);

    lastMessageStartedAt = system::getTimeSinceBootMsec();
    for (int i = 0; i < NUM_SCINTILLATORS; i++) {
        startHistogramForScintillator(i);
        addZerosToHistogram(64 * (i + 1) - 1);
        addNonzeroToHistogram(4096);
        addZerosToHistogram(256 - (64 * (i + 1)));
        if (packHistogram()) {
            logSCMMeasurement();
            sendSCMMeasurement(payload);
        }
    }

    updated = true;
#else
    uint8_t ch;
    uint32_t value;
    uint8_t overlong;
    uint16_t bytes_read;

    updated = false;
    if (uart::read(uart::SCM, &ch, 1, &bytes_read)) {
        if (varuint_decoder_feed(&varuintDecoder, ch)) {
            value = varuint_decoder_get_value(&varuintDecoder);
            overlong = varuint_decoder_get_overlong(&varuintDecoder);

            if (overlong == 2 && value < NUM_SCINTILLATORS) {
                /* Marker for new scintillator histogram */
                if (lastMessageStartedAt > 0) {
                    /* Log previous histogram */
                    if (packHistogram()) {
                        logSCMMeasurement();
                        sendSCMMeasurement(payload);
                    }
                }
                startHistogramForScintillator(value);
            } else if (overlong == 1 && value == 0) {
                /* Start of new packet */
                lastMessageStartedAt = system::getTimeSinceBootMsec();
                updated = true;
            } else if (overlong == 1) {
                /* Run of zeros in current histogram */
                addZerosToHistogram(value);
            } else if (overlong == 0) {
                /* Nonzero value in current histogram */
                addNonzeroToHistogram(value);
            }
        }
    }
#endif

    return updateStatus();
}

}

/**
 * @brief Starts processing the histogram for the scintillator with the given index.
 *
 * @param index  index of the scintillator
 */
static void startHistogramForScintillator(uint8_t index)
{
    scintillatorIndex = index;
    histogramIndex = 0;
    memset(histogram, 0, HISTOGRAM_SIZE * sizeof(uint16_t));
}

/**
 * @brief Adds a new nonzero entry to the current histogram.
 *
 * This function is a no-op if we have reached the end of the histogram.
 */
static void addNonzeroToHistogram(uint32_t value)
{
    if (histogramIndex >= HISTOGRAM_SIZE) {
        return;
    }

    if (value >= std::numeric_limits<uint16_t>::max()) {
        value = std::numeric_limits<uint16_t>::max();
    }
    histogram[histogramIndex] = value;
    histogramIndex++;
}

/**
 * @brief Adds a run of zero entries to the current histogram.
 *
 * This function is a no-op if we have reached the end of the histogram.
 */
static void addZerosToHistogram(uint32_t count)
{
    while (count > 0 && histogramIndex < HISTOGRAM_SIZE) {
        histogram[histogramIndex] = 0;
        histogramIndex++;
        count--;
    }
}

/**
 * @brief Encodes the current histogram into its wire format.
 * @return whether the encoding was successful. The encoding may fail if there
 * is not enough space in the pre-allocated buffer to store the entire encoded
 * histogram.
 */
static bool packHistogram()
{
    size_t size = histogram_get_packed_size(histogram, HISTOGRAM_SIZE);
    if (size > sizeof(packedHistogram)) {
        packedHistogramEnd = nullptr;
        return false;
    } else {
        packedHistogramEnd = histogram_pack(packedHistogram, histogram, HISTOGRAM_SIZE);
        return true;
    }
}

/**
 * @brief Stores a new SCM measurement in the log files.
 */
static void logSCMMeasurement()
{
    size_t length = packedHistogramEnd - packedHistogram;
    uint8_t* ptr;
    int i, numFragments = length / FRAGMENT_SIZE;

    if (length % FRAGMENT_SIZE > 0) {
        numFragments++;
    }

    for (i = 0, ptr = packedHistogram; i < numFragments; i++) {
        length = packedHistogramEnd - ptr;
        if (length > FRAGMENT_SIZE) {
            length = FRAGMENT_SIZE;
        }

        scmFrame.fragmentIndex = i;
        scmFrame.length = length;

        logRecord.write(
            lastMessageStartedAt,
            scintillatorIndex,
            numFragments - 1,
            i,
            length,
            ptr);
    }
}

/**
 * @brief Sends a new SCM telemetry message.
 *
 * @param payload   a buffer in which the message can be assembled
 */
static void sendSCMMeasurement(uint8_t* payload)
{
    size_t length = packedHistogramEnd - packedHistogram;
    uint8_t* ptr;
    int i, numFragments = length / FRAGMENT_SIZE;

    if (length % FRAGMENT_SIZE > 0) {
        numFragments++;
    }

    if (numFragments == 0) {
        return;
    }

    scmFrame.timestampInMsec = lastMessageStartedAt;
    scmFrame.scintillatorIndex = scintillatorIndex;
    scmFrame.maxFragmentIndex = numFragments - 1;

    for (i = 0, ptr = packedHistogram; i < numFragments; i++) {
        length = packedHistogramEnd - ptr;
        if (length > FRAGMENT_SIZE) {
            length = FRAGMENT_SIZE;
        }

        scmFrame.fragmentIndex = i;
        scmFrame.length = length;

        memcpy(scmFrame.data, ptr, length);
        ptr += length;

        /* TODO(ntamas): count how many messages are skipped */
        length = frames::encodeSCMFrame(&scmFrame, payload);
        send(frames::SCM, payload, length, 20);
    }
}

/**
 * Updates the subsystem status based on the timestamp of the last
 * received message from the SCM.
 */
static bool updateStatus()
{
    uint32_t now = system::getTimeSinceBootMsec();

    if (lastMessageStartedAt == 0) {
        /* No messages yet */
        status = SUBSYSTEM_STATUS_ERROR;
    } else if (now - lastMessageStartedAt < 300) {
        status = SUBSYSTEM_STATUS_OK;
    } else if (now - lastMessageStartedAt < 3000) {
        status = SUBSYSTEM_STATUS_WARNING;
    } else {
        status = SUBSYSTEM_STATUS_ERROR;
    }
    return status != SUBSYSTEM_STATUS_ERROR;
}
