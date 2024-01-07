#pragma once

#include <cmsis_os2.h>

namespace teller::tasks {

extern const osThreadAttr_t blinkTaskAttr;
void blinkTask(void* args);

}
