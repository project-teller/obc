#pragma once

#include <cmsis_os2.h>

namespace teller::tasks {
extern const osThreadAttr_t serialTaskAttr;
void serialTask(void* args);
}
