#pragma once

#include <cmsis_os2.h>

namespace teller::tasks {

extern const osThreadAttr_t supervisorTaskAttr;
void supervisorTask(void* args);

}
