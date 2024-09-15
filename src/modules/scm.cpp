#include <algorithm>
#include <minmea.h>

#include "core/log_records.h"
#include "core/nmea/parser.h"

#include "hal/system.h"
#include "hal/uart.h"

#include "modules/edr.hpp"
#include "modules/log.h"
#include "modules/scm.h"
#include "modules/telem.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::math;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

// static frames::gmm_data_t measurement;

static teller::edr::FormattedLogRecord<
    uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t*>
    logRecord(
        LOG_RECORD_SCM, "SCM",
        "TimeMS,I,NumFrags,Frag,Length,Histogram",
        "IBBBBZ", "s-----", "C-----");

static Logger* logger;
static teller::nmea::Parser parser;
static uint32_t lastMessageStartedAt;
static uint32_t lastMessageReceivedAt;

static void logSCMMeasurement(void);
static bool parseReceivedMessage(const char* message);
static void sendSCMMeasurement(uint8_t* payload);
static bool updateStatus(void);

namespace teller::scm {

bool init()
{
    parser.reset();

    status = SUBSYSTEM_STATUS_CRITICAL;
    logger = getLogger(MODULE_ID_SCM);

    lastMessageStartedAt = 0;
    lastMessageReceivedAt = 0;

    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
    status = SUBSYSTEM_STATUS_CRITICAL;
}

subsystem_status_t getSubsystemStatus()
{
    return status;
}

bool setup()
{
    lastMessageReceivedAt = system::getTimeSinceBootMsec();
    status = uart::isConnected(uart::SCM) ? SUBSYSTEM_STATUS_OK : SUBSYSTEM_STATUS_CRITICAL;
    return updateStatus();
}

bool update(uint8_t* payload, bool& updated)
{
    uint8_t ch;

    updated = false;
    if (uart::read1(uart::SCM, &ch, 500)) {
        if (ch == '$') {
            lastMessageStartedAt = system::getTimeSinceBootMsec();
        }
        if (parser.feed(ch) && parseReceivedMessage(parser.getMessage())) {
            lastMessageReceivedAt = system::getTimeSinceBootMsec();
            logSCMMeasurement();
            sendSCMMeasurement(payload);
            updated = true;
        }
    }

    return updateStatus();
}

}

/**
 * @brief Stores a new SCM measurement in the log files.
 */
static void logSCMMeasurement()
{
    /*
    logRecord.write(
        measurement.timestampInMsec,
        measurement.hitCounts.byIndex[0],
        measurement.hitCounts.byIndex[1],
        measurement.hitCounts.byIndex[2],
        measurement.hitCounts.byIndex[3],
        measurement.hitCounts.byIndex[4],
        measurement.hitCounts.byIndex[5],
        measurement.hitCounts.byIndex[6],
        measurement.hitCounts.byIndex[7],
        measurement.hitCounts.byIndex[8],
        measurement.hitCounts.byIndex[9]);
    */
}

/**
 * Attempts to parse a received NMEA sentence to see if it contains an
 * SCM histogram. Updates the current histogram in case of a match.
 */
static bool parseReceivedMessage(const char* message)
{
    return false;

    /*
    char type[6];
    int counts[10];

    if (!minmea_scan(
            message, "tiiiiiiiiii", type,
            &counts[0], &counts[1], &counts[2], &counts[3], &counts[4],
            &counts[5], &counts[6], &counts[7], &counts[8], &counts[9])) {
        return false;
    }

    measurement.timestampInMsec = lastMessageStartedAt;
    for (int i = 0; i < 10; i++) {
        measurement.hitCounts.byIndex[i] = counts[i];
    }

    return true;
    */
}

/**
 * @brief Sends a new SCM telemetry message.
 *
 * @param payload   a buffer in which the message can be assembled
 */
static void sendSCMMeasurement(uint8_t* payload)
{
}

/**
 * Updates the subsystem status based on the timestamp of the last
 * received message from the GMM.
 */
static bool updateStatus()
{
    uint32_t now = system::getTimeSinceBootMsec();
    if (now - lastMessageReceivedAt < 100) {
        status = SUBSYSTEM_STATUS_OK;
    } else if (now - lastMessageReceivedAt < 3000) {
        status = SUBSYSTEM_STATUS_WARNING;
    } else {
        /* This is where we are giving up */
        status = SUBSYSTEM_STATUS_ERROR;
    }
    return status != SUBSYSTEM_STATUS_ERROR;
}
