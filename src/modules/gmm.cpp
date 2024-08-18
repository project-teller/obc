#include <algorithm>
#include <minmea.h>

#include "core/gmm/parser.h"

#include "hal/system.h"
#include "hal/uart.h"

#include "modules/edr.hpp"
#include "modules/gmm.h"
#include "modules/log.h"

using namespace teller::hal;
using namespace teller::log;
using namespace teller::math;
using namespace teller::telem;

static subsystem_status_t status = SUBSYSTEM_STATUS_CRITICAL;

static measurement_gmm_t hitCounts;

static teller::edr::FormattedLogRecord<
    uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>
    logRecord(
        2, "GMM", "TimeMS,Cnt1,Cnt2,Cnt3,Cnt4,Cnt12,Cnt13,Cnt14,Cnt23,Cnt24,Cnt34",
        "IBBBBBBBBBB", "s----------", "C0000000000");

static Logger* logger;
static teller::gmm::Parser parser;
static uint32_t lastMessageReceivedAt;

static bool parseReceivedMessage(const char* message);
static bool updateStatus();

namespace teller::gmm {

bool init()
{
    parser.reset();

    status = SUBSYSTEM_STATUS_CRITICAL;
    logger = getLogger(MODULE_ID_GMM);
    lastMessageReceivedAt = 0;

    return logger != nullptr;
}

void destroy()
{
    logger = nullptr;
    status = SUBSYSTEM_STATUS_CRITICAL;
}

measurement_gmm_t getHitCounts(void)
{
    return hitCounts;
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

bool update()
{
    uint8_t ch;

    if (
        uart::read1(uart::GMM, &ch, 500) && parser.feed(ch) && parseReceivedMessage(parser.getMessage())) {
        lastMessageReceivedAt = system::getTimeSinceBootMsec();
    }

    return updateStatus();
}

void log()
{
    logRecord.write(
        hitCounts.timestampInMsec,
        hitCounts.hitCounts.byIndex[0],
        hitCounts.hitCounts.byIndex[1],
        hitCounts.hitCounts.byIndex[2],
        hitCounts.hitCounts.byIndex[3],
        hitCounts.hitCounts.byIndex[4],
        hitCounts.hitCounts.byIndex[5],
        hitCounts.hitCounts.byIndex[6],
        hitCounts.hitCounts.byIndex[7],
        hitCounts.hitCounts.byIndex[8],
        hitCounts.hitCounts.byIndex[9]);
}
}

/**
 * Attempts to parse a received NMEA sentence to see if it contains
 * GM hit counts. Updates the hit counts in case of a match.
 */
static bool parseReceivedMessage(const char* message)
{
    char type[6];
    int counts[10];

    return true;

    if (!minmea_scan(
            message, "tiiiiiiiiii", type,
            &counts[0], &counts[1], &counts[2], &counts[3], &counts[4],
            &counts[5], &counts[6], &counts[7], &counts[8], &counts[9])) {
        return false;
    }

    /* TODO: it would be more accurate to return the time when the last dollar
     * sign was received as it is closer to the time when the measurement was
     * taken */
    hitCounts.timestampInMsec = system::getTimeSinceBootMsec();
    for (int i = 0; i < 10; i++) {
        hitCounts.hitCounts.byIndex[i] = counts[i];
    }

    return true;
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
