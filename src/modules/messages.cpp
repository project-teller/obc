#include "hal/rtc.h"
#include "hal/system.h"

#include "modules/errors.h"
#include "modules/messages.h"
#include "modules/rxsm.h"

using namespace teller::errors;
using namespace teller::hal;
using namespace teller::telem;

void teller::telem::updateHeartbeatData(frames::heartbeat_data_t* data)
{
    teller::rxsm::State state;

    teller::rxsm::getState(state);

    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->error = getError();

    data->rxsmStatusBits.lo = state.lo;
    data->rxsmStatusBits.sods = state.sods;
    data->rxsmStatusBits.soe = state.soe;
}

void teller::telem::updateTimesyncData(frames::timesync_data_t* data)
{
    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->rtcTimestampInMsec = rtc::getTimeMsec();
}
