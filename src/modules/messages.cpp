#include <algorithm>

#include "drivers/imu.h"

#include "hal/board.h"
#include "hal/rtc.h"
#include "hal/system.h"

#include "modules/adc.h"
#include "modules/cam.h"
#include "modules/errors.h"
#include "modules/gmm.h"
#include "modules/imu.h"
#include "modules/lcl.h"
#include "modules/mag.h"
#include "modules/messages.h"
#include "modules/mode.h"
#include "modules/rxsm.h"
#include "modules/scheduler.h"
#include "modules/scm.h"
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
    data->temperatureInCelsius = board::getBoardTemperature();

    data->rxsmStatusBits.lo = state.lo;
    data->rxsmStatusBits.sods = state.sods;
    data->rxsmStatusBits.soe = state.soe;

    data->subsystemStatus.cam = teller::cam::getSubsystemStatus();
    data->subsystemStatus.gmm = teller::gmm::getSubsystemStatus();
    data->subsystemStatus.scm = teller::scm::getSubsystemStatus();
    data->subsystemStatus.imu = teller::imu::getSubsystemStatus();
    data->subsystemStatus.mag = teller::mag::getSubsystemStatus();
    data->subsystemStatus.sto = teller::storage::getSubsystemStatus();

    data->lclStatusBits.gmm = teller::lcl::triggered(teller::lcl::GMM_LCL);
    data->lclStatusBits.scm = teller::lcl::triggered(teller::lcl::SCM_LCL);
    data->lclStatusBits.suc1 = teller::lcl::triggered(teller::lcl::SUC_LCL1);
    data->lclStatusBits.suc2 = teller::lcl::triggered(teller::lcl::SUC_LCL2);
    data->lclStatusBits.suc3 = teller::lcl::triggered(teller::lcl::SUC_LCL3);
    data->lclStatusBits.cam = teller::lcl::triggered(teller::lcl::CAM_LCL);
}

void updateClockStatusData(frames::clock_status_data_t* data)
{
    rxsm::State state;

    data->timestampInMsec = system::getTimeSinceBootMsec();
    data->rtcTimestampInMsec = rtc::getTimeMsec();

    data->missionClockInMsec = scheduler::getMissionClock()->getElapsedTimeMsec();
    data->missionClockIsRunning = scheduler::getMissionClock()->isRunning();

    rxsm::getState(state);
    data->liftoffTimestampInMsec = rxsm::getTimeSinceLiftoffMsec();
    data->liftoffHappened = state.lo;
}

void updateIMUMeasurement(frames::imu_data_t* data)
{
    measurement_3d_t measurement;

    measurement = teller::imu::getAcceleration();
    data->timestampInMsec = measurement.timestampInMsec;
    data->acceleration = measurement.value;

    measurement = teller::imu::getAngularVelocity();
    data->timestampInMsec = std::max(data->timestampInMsec, measurement.timestampInMsec);
    data->angularVelocity = measurement.value;
}

void updateMagneticVectorMeasurement(frames::mag_data_t* data)
{
    measurement_3d_t measurement;

    measurement = teller::mag::getMagneticVector();
    data->timestampInMsec = measurement.timestampInMsec;
    data->magneticVector = measurement.value;
    data->temperature = teller::mag::getTemperature();
}

void updateADCMeasurements(frames::adc_data_t* data)
{
    teller::adc::getMeasurements(
        data->timestampInMsec,
        data->measurements.byIndex);
}

}
