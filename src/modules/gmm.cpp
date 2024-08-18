#include <algorithm>
#include <minmea.h>

#include "core/gmm/parser.h"
#include "core/telem/gmm.h"

#include "hal/system.h"
#include "hal/uart.h"

#include "modules/edr.hpp"
#include "modules/gmm.h"
#include "modules/log.h"
#include "modules/telem.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::math;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

static frames::gmm_data_t measurement;

static teller::edr::FormattedLogRecord<
    uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>
    logRecord(
        2, "GMM", "TimeMS,Cnt1,Cnt2,Cnt3,Cnt4,Cnt12,Cnt13,Cnt14,Cnt23,Cnt24,Cnt34",
        "IBBBBBBBBBB", "s----------", "C0000000000");

static Logger* logger;
static teller::gmm::Parser parser;
static uint32_t lastMessageStartedAt;
static uint32_t lastMessageReceivedAt;

static void logGMMMeasurement(void);
static bool parseReceivedMessage(const char* message);
static void sendGMMMeasurement(uint8_t* payload);
static bool updateStatus(void);

namespace teller::gmm {

bool init()
{
    parser.reset();

    status = SUBSYSTEM_STATUS_CRITICAL;
    logger = getLogger(MODULE_ID_GMM);

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
    status = SUBSYSTEM_STATUS_OK;
    lastMessageReceivedAt = system::getTimeSinceBootMsec();
    return updateStatus();
}

bool update(uint8_t* payload, bool& updated)
{
    uint8_t ch;

    updated = false;
    if (uart::read1(uart::GMM, &ch, 500)) {
        if (ch == '$') {
            lastMessageStartedAt = system::getTimeSinceBootMsec();
        }
        if (parser.feed(ch) && parseReceivedMessage(parser.getMessage())) {
            lastMessageReceivedAt = system::getTimeSinceBootMsec();
            logGMMMeasurement();
            sendGMMMeasurement(payload);
            updated = true;
        }
    }

    return updateStatus();
}

}

/**
 * @brief Stores a new GMM measurement in the log files.
 */
static void logGMMMeasurement()
{
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
}

/**
 * Attempts to parse a received NMEA sentence to see if it contains
 * GM hit counts. Updates the hit counts in case of a match.
 */
static bool parseReceivedMessage(const char* message)
{
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
}

/**
 * @brief Sends a new GMM telemetry message.
 *
 * @param payload   a buffer in which the message can be assembled
 */
static void sendGMMMeasurement(uint8_t* payload)
{
    /* We want to send the telemetry message as fast as possible so we can read
     * the next message in time, hence the short timeout */
    uint8_t length = frames::encodeGMMFrame(&measurement, payload);

    /* TODO(ntamas): count how many messages are skipped */
    send(frames::GMM, payload, length, 20);
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
