#pragma once

#include "core/telem/clock_status.h"
#include "core/telem/gmm.h"
#include "core/telem/heartbeat.h"
#include "core/telem/imu.h"
#include "core/telem/mag.h"

namespace teller::telem {

/**
 * @brief Updates a heartbeat data object from the current system status.
 */
void updateHeartbeatData(frames::heartbeat_data_t* data);

/**
 * @brief Updates a clock status data object from the current system and RTC time.
 */
void updateClockStatusData(frames::clock_status_data_t* data);

/**
 * @brief Updates an IMU measurement data object from the most recent measurement
 * of the IMU.
 */
void updateIMUMeasurement(frames::imu_data_t* data);

/**
 * @brief Updates a MAG measurement data object from the most recent measurement
 * of the magnetometer.
 */
void updateMagneticVectorMeasurement(frames::mag_data_t* data);

}
