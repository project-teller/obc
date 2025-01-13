#pragma once

namespace teller::log {

typedef enum {
    LOG_RECORD_BRD = 1,
    LOG_RECORD_CLK = 2,
    LOG_RECORD_ERR = 3,
    LOG_RECORD_GMM = 4,
    LOG_RECORD_IMU = 5,
    LOG_RECORD_LCL = 6,
    LOG_RECORD_LCLV = 7,
    LOG_RECORD_LCLI = 8,
    LOG_RECORD_LOG = 9,
    LOG_RECORD_MAG = 10,
    LOG_RECORD_RXSM = 11,
    LOG_RECORD_SCM = 12,
    LOG_RECORD_SYS = 13,
} log_record_id_t;

}
