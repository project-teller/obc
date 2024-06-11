#include <algorithm>

#include "hal/board.h"
#include "hal/imu.h"
#include "hal/rtc.h"
#include "hal/system.h"

#include "modules/errors.h"
#include "modules/imu.h"
#include "modules/lcl.h"
#include "modules/messages.h"
#include "modules/mode.h"
#include "modules/rxsm.h"
#include "modules/storage.h"

using namespace teller::errors;
using namespace teller::hal;
using namespace teller::mode;
using namespace teller::telem;

namespace teller::telem {

void updateHeartbeatData(frames::heartbeat_data_t* data)
{
    teller::rxsm::State state;

    teller::rxsm::getState(state);

    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->error = getError();

    data->mode = getMode();

    data->voltageInVolts = board::getBoardVoltage();
    data->temperateInCelsius = board::getBoardTemperature();

    data->rxsmStatusBits.lo = state.lo;
    data->rxsmStatusBits.sods = state.sods;
    data->rxsmStatusBits.soe = state.soe;

    data->subsystemStatus.imu = teller::imu::getSubsystemStatus();
    data->subsystemStatus.sto = teller::storage::getSubsystemStatus();

    data->lclStatusBits.gmm = teller::lcl::triggered(teller::lcl::GMM_LCL);
    data->lclStatusBits.scm = teller::lcl::triggered(teller::lcl::SCM_LCL);
    data->lclStatusBits.suc1 = teller::lcl::triggered(teller::lcl::SUC_LCL1);
    data->lclStatusBits.suc2 = teller::lcl::triggered(teller::lcl::SUC_LCL2);
    data->lclStatusBits.suc3 = teller::lcl::triggered(teller::lcl::SUC_LCL3);
    data->lclStatusBits.hvpsu = teller::lcl::triggered(teller::lcl::HVPSU_LCL);
}

void updateClockStatusData(frames::clock_status_data_t* data)
{
    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->rtcTimestampInMsec = rtc::getTimeMsec();
}

void updateIMUMeasurement(frames::imu_data_t* data)
{
    teller::hal::imu::measurement_t measurement;

    teller::hal::imu::getAcceleration(measurement);
    data->timestampInMsec = measurement.timestampInMsec;
    data->acceleration.x = measurement.x;
    data->acceleration.y = measurement.y;
    data->acceleration.z = measurement.z;

    teller::hal::imu::getAngularVelocity(measurement);
    data->timestampInMsec = std::max(data->timestampInMsec, measurement.timestampInMsec);
    data->angularVelocity.x = measurement.x;
    data->angularVelocity.y = measurement.y;
    data->angularVelocity.z = measurement.z;
}

}
