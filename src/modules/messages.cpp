#include "hal/rtc.h"
#include "hal/system.h"

#include "modules/errors.h"
#include "modules/lcl.h"
#include "modules/messages.h"
#include "modules/rxsm.h"
#include "modules/storage.h"

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

    data->subsystemStatus.sto = teller::storage::getSubsystemStatus();

    data->lclStatusBits.gmm = teller::lcl::triggered(teller::lcl::GMM_LCL);
    data->lclStatusBits.scm = teller::lcl::triggered(teller::lcl::SCM_LCL);
    data->lclStatusBits.suc1 = teller::lcl::triggered(teller::lcl::SUC_LCL1);
    data->lclStatusBits.suc2 = teller::lcl::triggered(teller::lcl::SUC_LCL2);
    data->lclStatusBits.suc3 = teller::lcl::triggered(teller::lcl::SUC_LCL3);
    data->lclStatusBits.hvpsu = teller::lcl::triggered(teller::lcl::HVPSU_LCL);
}

void teller::telem::updateClockStatusData(frames::clock_status_data_t* data)
{
    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->rtcTimestampInMsec = rtc::getTimeMsec();
}
