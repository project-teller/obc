#include "config.h"

#include "hal/board.h"
#include "hal/hal.h"
#include "hal/system.h"

#include "modules/cmd.h"
#include "modules/edr.hpp"
#include "modules/errors.h"
#include "modules/imu.h"
#include "modules/lcl.h"
#include "modules/log.h"
#include "modules/mode.h"
#include "modules/rxsm.h"
#include "modules/storage.h"
#include "modules/telem.h"

#include "tasks/blinker.h"
#include "tasks/cmd.h"
#include "tasks/debug.h"
#include "tasks/flashmem.h"
#include "tasks/imu.h"
#include "tasks/logger.h"
#include "tasks/mode.h"
#include "tasks/pins.h"
#include "tasks/sdcard.h"
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

static cmd_task_args_t cmd_task_args = {
    .uart_index = teller::hal::uart::TELEMETRY
};

static const task_definition_t tasks[] = {
    { .func = blinkTask, .name = "blinker", .priority = LOW },
    { .func = pinsTask, .name = "pins", .priority = NORMAL, .stack_size = 1024 },
    { .func = serialTask, .name = "serial", .priority = HIGH, .stack_size = 1024 },
    { .func = supervisorTask, .name = "supervisor", .priority = LOW },
    { .func = telemetryTask, .name = "telem", .priority = NORMAL, .stack_size = 1024 },
    { .func = commandTask, .name = "cmd", .priority = LOW, .stack_size = 1024, .context = &cmd_task_args },
    { .func = flashMemoryTask, .name = "flashmem", .priority = HIGH, .stack_size = 1024 },
    { .func = sdCardTask, .name = "sdcard", .priority = HIGH, .stack_size = 1024 },
    { .func = imuTask, .name = "imu", .priority = NORMAL, .stack_size = 1024 },
    { .func = loggerTask, .name = "log", .priority = NORMAL, .stack_size = 1024 },
    { .func = modeManagerTask, .name = "mode", .priority = NORMAL, .stack_size = 1024 },
    { .func = debugTask, .name = "debug", .priority = NORMAL, .stack_size = 1024 },
    NO_MORE_TASKS
};

static void initialize(void);
static bool startTasks(void);
static void runScheduler(void);

#ifdef TELLER_BOARD_POSIX
namespace teller::hal::uart {
void setDebugPort(const std::string&);
}
#endif

void bootSystem(void)
{
    bool inited;

    initialize();

    inited = teller::hal::init();

    /* We start the error handler module no matter what. In the worst case, the
     * error LED won't work but at least we are still logging the errors */
    teller::errors::init();

    /* The remaining modules are initialized only if the HAL initialization
     * was successful */
    inited &= teller::log::init();
    inited &= teller::lcl::init();
    inited &= teller::mode::init();
    inited &= teller::rxsm::init();
    inited &= teller::storage::init();
    inited &= teller::telem::init();
    inited &= teller::cmd::init();
    inited &= teller::edr::init();
    inited &= teller::imu::init();

    /* The remaining tasks are started only if the HAL and the module
     * initialization was successful */
    inited &= startTasks();

    /* Set an error code if the initialization failed */
    if (!inited) {
        teller::errors::setError(teller::errors::SYSTEM_INIT_ERROR);
    }

    runScheduler();
}

#ifdef TELLER_BOARD_POSIX

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <thread>

static void parentPipeWatcher(int fd);

int main(void)
{
    pid_t pid;
    int result = 0;
    int pipes[2];
    bool shouldBoot = true;

    srand(time(NULL));

    /* Create a pipe that will be used to allow the child process to detect
     * when the parent died. When the parent dies, the pipe is closed, and
     * this wil lbe detected by the child. */
    if (pipe(pipes)) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    while (shouldBoot) {
        pid = fork();

        switch (pid) {
        case -1:
            /* fork failed */
            perror("fork");
            return EXIT_FAILURE;

        case 0:
            /* Child process. Close the write end of the pipe, fork a thread
             * to read from the read end. */

            // Prevent broken sockets from causing SIGPIPE signals
            signal(SIGPIPE, SIG_IGN);
            close(pipes[1]);

            {
                std::thread t(parentPipeWatcher, pipes[0]);
                t.detach();
                bootSystem();
            }
            return EXIT_SUCCESS;

        default:
            /* Parent process. Close the read end of the pipe, keep the write
             * end open. */
            close(pipes[0]);

            /* Wait for the child and decide based on the return code. */
            if (waitpid(pid, &result, 0) < 0) {
                perror("waitpid");
                return EXIT_FAILURE;
            }

            /* Restart if the child received a SIGUSR1, otherwise stop */
            shouldBoot = WIFSIGNALED(result) && WTERMSIG(result) == SIGUSR1;
        }
    }

    return WIFEXITED(result) ? WEXITSTATUS(result) : 0;
}

static void parentPipeWatcher(int fd)
{
    char buf;

    while (true) {
        while (read(fd, &buf, 1) > 0)
            ;

        if (errno == 0) {
            close(fd);
            kill(getpid(), SIGTERM);
            sleep(3);
            kill(getpid(), SIGKILL);
        }
    }
}

static void initialize()
{
    std::string DEBUG_PORT("4747");

    std::cerr << "Opening debug port on port " << DEBUG_PORT << "... " << std::flush;
    teller::hal::uart::setDebugPort(DEBUG_PORT);
    std::cerr << " done." << std::endl;

    std::cerr << "TELLER OBC initializing... " << std::flush;
}

static void runScheduler()
{
    std::cerr << " done." << std::endl;

    for (;;) {
        teller::hal::system::delayMsec(60000);
    }
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

int main(void)
{
    bootSystem();
    return 0;
}

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
