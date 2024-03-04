#include "config.h"
#include "hal/hal.h"
#include "modules/errors.h"
#include "modules/telem.h"
#include "tasks/blinker.h"
#include "tasks/pins.h"
#include "tasks/serial.h"
#include "tasks/supervisor.h"
#include "tasks/telem.h"

using namespace teller::tasks;

/**
 * @brief Typedef for the function signature of a task in the OBC.
 */
typedef void task_t(void*);

/**
 * @brief Enum containing the different priorities defined for tasks in the OBC.
 */
typedef enum {
    LOW,
    NORMAL,
    HIGH
} task_priority_t;

/**
 * @brief Struct holding the attributes required to start a task in the OBC.
 */
typedef struct {
    task_t* func;
    const char* name;
    task_priority_t priority;
    uint32_t stack_size;
    void* context;
} task_definition_t;

#define NO_MORE_TASKS \
    {                 \
        0             \
    }

static const task_definition_t tasks[] = {
    { .func = blinkTask, .name = "blinker", .priority = LOW },
    { .func = pinsTask, .name = "pins", .priority = NORMAL },
    { .func = serialTask, .name = "serial", .priority = HIGH, .stack_size = 1024 },
    { .func = supervisorTask, .name = "supervisor", .priority = LOW },
    { .func = telemetryTask, .name = "telem", .priority = NORMAL, .stack_size = 1024 },
    NO_MORE_TASKS
};

static void initialize(void);
static bool startTasks(void);
static void runScheduler(void);

int main(void)
{
    bool inited;

    initialize();

    inited = teller::hal::init();

    /* We start the error handler module no matter what. In the worst case, the
     * error LED won't work but at least we are still logging the errors */
    teller::errors::init();

    /* The remaining modules are initialized only if the HAL initialization
     * was successful */
    inited &= teller::telem::init();

    /* The remaining tasks are started only if the HAL and the module
     * initialization was successful */
    inited &= startTasks();

    /* Set an error code if the initialization failed */
    if (!inited) {
        teller::errors::setError(teller::errors::SYSTEM_INIT_ERROR);
    }

    runScheduler();

    return 0;
}

#ifdef TELLER_BOARD_POSIX

#include <iostream>
#include <thread>

static void initialize()
{
    std::cerr << "TELLER OBC initializing... " << std::flush;
}

static void runScheduler()
{
    std::cerr << " done." << std::endl;
    for (;;)
        ;
}

static bool startTasks()
{
    for (const task_definition_t* task = tasks; task->func; task++) {
        std::thread t(task->func, task->context);
        t.detach();
    }
    return true;
}

#else

/* ************************************************************************* */
/* FreeRTOS-specific part follows here */

#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <task.h>

static osPriority_t convertPriorityToFreeRTOS(task_priority_t prio);
static bool startTask(const task_definition_t* task);

static void initialize()
{
    osKernelInitialize();
}

static bool startTasks()
{
    bool success = true;
    for (const task_definition_t* task = tasks; task->func; task++) {
        if (!startTask(task)) {
            success = false;
        }
    }
    return success;
}

static void runScheduler()
{
    osKernelStart();
    for (;;)
        ;
}

static osPriority_t convertPriorityToFreeRTOS(task_priority_t prio)
{
    switch (prio) {
    case LOW:
        return osPriorityLow;
    case NORMAL:
        return osPriorityNormal;
    case HIGH:
        return osPriorityHigh;
    default:
        return osPriorityLow;
    }
}

static bool startTask(const task_definition_t* task)
{
    osThreadAttr_t attr = {
        .name = task->name,
        .stack_size = task->stack_size,
        .priority = convertPriorityToFreeRTOS(task->priority),
    };

    return osThreadNew(task->func, task->context, &attr) != nullptr;
}

extern "C" void vApplicationTickHook(void)
{
}

extern "C" void vApplicationIdleHook(void)
{
}

extern "C" void vApplicationMallocFailedHook(void)
{
    /* TODO: store the event in a .noinit variable so we can report it at next
     * boot. Maybe also auto-reset? */
    teller::hal::notifyFatalError();

    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;

    /* TODO: store the event in a .noinit variable so we can report it at next
     * boot. Maybe also auto-reset? */

    teller::hal::notifyFatalError();

    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}

#endif
