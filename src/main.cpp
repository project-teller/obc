#include "config.h"

#include "hal/board.h"
#include "hal/hal.h"
#include "hal/system.h"

#include "drivers/adc.h"
#include "drivers/flashmem.h"
#include "drivers/imu.h"
#include "drivers/mag.h"
#include "drivers/sdcard.h"
#include "drivers/temperature.h"

#include "modules/adc.h"
#include "modules/cam.h"
#include "modules/cmd.h"
#include "modules/debug.h"
#include "modules/edr.hpp"
#include "modules/errors.h"
#include "modules/gmm.h"
#include "modules/imu.h"
#include "modules/lcl.h"
#include "modules/log.h"
#include "modules/mag.h"
#include "modules/mode.h"
#include "modules/rxsm.h"
#include "modules/scheduler.h"
#include "modules/scm.h"
#include "modules/storage.h"
#include "modules/supervisor.h"
#include "modules/telem.h"
#include "modules/uart_rx.h"

#include "tasks/adc.h"
#include "tasks/blinker.h"
#include "tasks/cmd.h"
#include "tasks/debug.h"
#include "tasks/flashmem.h"
#include "tasks/gmm.h"
#include "tasks/imu.h"
#include "tasks/logger.h"
#include "tasks/mag.h"
#include "tasks/mission.h"
#include "tasks/mode.h"
#include "tasks/pins.h"
#include "tasks/scm.h"
#include "tasks/sdcard.h"
#include "tasks/storage.h"
#include "tasks/supervisor.h"
#include "tasks/telem.h"
#include "tasks/temperature.h"
#include "tasks/uart_rx.h"
#include "tasks/uart_tx.h"
#include "tasks/usb.h"

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

static uart_rx_task_args_t uart_telem_task_args = {
    .task_name = "uartRxTel",
    .uart_index = teller::hal::uart::RXSM
};

static uart_rx_task_args_t uart_debug_task_args = {
    .task_name = "uartRxDbg",
    .uart_index = teller::hal::uart::DEBUG
};

static const task_definition_t tasks[] = {
    { .func = blinkTask, .name = "blinker", .priority = LOW, .stack_size = 1024 },
    { .func = pinsTask, .name = "pins", .priority = NORMAL, .stack_size = 1024 },
    { .func = temperatureTask, .name = "temperature", .priority = LOW, .stack_size = 1024 },
    { .func = uartTxTask, .name = "uartTx", .priority = HIGH, .stack_size = 2048 },
    { .func = supervisorTask, .name = "supervisor", .priority = LOW, .stack_size = 1024 },
    { .func = telemetryTask, .name = "telem", .priority = NORMAL, .stack_size = 2048 },
    { .func = commandTask, .name = "cmd", .priority = LOW, .stack_size = 4096 },
    { .func = flashMemoryTask, .name = "flashmem", .priority = HIGH, .stack_size = 4096 },
    { .func = sdCardTask, .name = "sdcard", .priority = HIGH, .stack_size = 4096 },
    { .func = imuTask, .name = "imu", .priority = NORMAL, .stack_size = 1024 },
    { .func = magTask, .name = "mag", .priority = NORMAL, .stack_size = 1024 },
    { .func = loggerTask, .name = "log", .priority = NORMAL, .stack_size = 2048 },
    { .func = modeManagerTask, .name = "mode", .priority = NORMAL, .stack_size = 1024 },
    { .func = debugTask, .name = "debug", .priority = NORMAL, .stack_size = 1024 },
    { .func = storageReaderTask, .name = "storage", .priority = LOW, .stack_size = 4096 },
    { .func = gmmTask, .name = "gmm", .priority = NORMAL, .stack_size = 1024 },
    { .func = scmTask, .name = "scm", .priority = NORMAL, .stack_size = 1024 },
    { .func = uartRxTask, .name = "uartRxTel", .priority = LOW, .stack_size = 1024, .context = &uart_telem_task_args },
    { .func = uartRxTask, .name = "uartRxDbg", .priority = LOW, .stack_size = 1024, .context = &uart_debug_task_args },
    { .func = usbTask, .name = "usb", .priority = LOW, .stack_size = 1024 },
    { .func = adcTask, .name = "adc", .priority = LOW, .stack_size = 2048 },
    { .func = missionTask, .name = "mission", .priority = LOW, .stack_size = 1024 },
    NO_MORE_TASKS
};

static void initialize(void);
static bool startTasks(void);
static void runScheduler(void);

#ifdef TELLER_BOARD_POSIX
namespace teller::hal::uart {
void setDebugPort(const std::string& service);
void setGMMFileDescriptor(int fd);
void setSCMFileDescriptor(int fd);
}
#endif

void bootSystem(void)
{
    bool inited;

    initialize();

    /* Start with initializing the debugging module. This is guaranteed to
     * succeed and it will be needed later for any sort of crashes; in
     * particular, stack overflows are logged there */
    teller::debug::init();

    inited = teller::hal::init();

    /* We start the error handler module no matter what. In the worst case, the
     * error LED won't work but at least we are still logging the errors */
    teller::errors::init();

    /* Initialize the drivers */
    inited &= teller::drivers::adc::init();
    inited &= teller::drivers::flashmem::init();
    inited &= teller::drivers::imu::init();
    inited &= teller::drivers::mag::init();
    inited &= teller::drivers::sdcard::init();
    inited &= teller::drivers::temperature::init();

    /* Initialize modules */
    inited &= teller::log::init();
    inited &= teller::supervisor::init();
    inited &= teller::adc::init();
    inited &= teller::cam::init();
    inited &= teller::lcl::init();
    inited &= teller::mode::init();
    inited &= teller::scheduler::init();
    inited &= teller::rxsm::init();
    inited &= teller::storage::init();
    inited &= teller::telem::init();
    inited &= teller::uart_rx::init();
    inited &= teller::cmd::init();
    inited &= teller::edr::init();
    inited &= teller::imu::init();
    inited &= teller::mag::init();
    inited &= teller::gmm::init();
    inited &= teller::scm::init();

    /* Start tasks */
    inited &= startTasks();

    /* Set an error code if the initialization failed */
    if (!inited) {
        teller::errors::setError(teller::errors::SYSTEM_INIT_ERROR);
    }

    runScheduler();
}

#ifdef TELLER_BOARD_POSIX

#include <minmea.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <thread>

static void parentPipeWatcher(int fd);
static void generateFakeGMMReadings(int fd);
static void generateFakeSCMReadings(int fd);

typedef union {
    int fds[2];
    struct {
        int rx;
        int tx;
    } by_role;
} pipe_pair_t;

int main(void)
{
    pid_t pid;
    int result = 0;
    struct {
        pipe_pair_t keepalive;
        pipe_pair_t gmm;
        pipe_pair_t scm;
    } pipes;
    bool shouldBoot = true;

    srand(time(NULL));

    /* Create a pipe that will be used to allow the child process to detect
     * when the parent died. When the parent dies, the pipe is closed, and
     * this will be detected by the child. */
    if (pipe(pipes.keepalive.fds)) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /* Create another pipe on which we will simulate fake readings from the
     * GMM. */
    if (pipe(pipes.gmm.fds)) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /* Create another pipe on which we will simulate fake readings from the
     * SCM. */
    if (pipe(pipes.scm.fds)) {
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

            // Close the write ends of the pipes that we do not need
            close(pipes.gmm.by_role.tx);
            close(pipes.scm.by_role.tx);
            close(pipes.keepalive.by_role.tx);

            // Connect the UART module in the HAL to the GMM
            teller::hal::uart::setGMMFileDescriptor(pipes.gmm.by_role.rx);

            // Connect the UART module in the HAL to the SCM
            teller::hal::uart::setSCMFileDescriptor(pipes.scm.by_role.rx);

            // Start a new thread to read the keepalive pipe. This is how we
            // will get notified if the parent dies.
            {
                std::thread t(parentPipeWatcher, pipes.keepalive.by_role.rx);
                t.detach();
                bootSystem();
            }
            return EXIT_SUCCESS;

        default:
            /* Parent process. Close the read ends of the pipes, keep the write
             * ends open. */
            close(pipes.gmm.by_role.rx);
            close(pipes.scm.by_role.rx);
            close(pipes.keepalive.by_role.rx);

            /* Start a new thread to simulate readings for the GMM */
            std::thread gmmThread(generateFakeGMMReadings, pipes.gmm.by_role.tx);
            gmmThread.detach();

            /* Start a new thread to simulate readings for the SCM */
            std::thread scmThread(generateFakeSCMReadings, pipes.scm.by_role.tx);
            scmThread.detach();

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

static void generateFakeGMMReadings(int fd)
{
    FILE* fp;
    const float gmmReportFreq = 50;
    int dt = 1000 / gmmReportFreq;
    int seq_no = 0;
    char message[128];

    fp = fdopen(fd, "w");

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(dt));

        /* Print the message... */
        snprintf(
            message, sizeof(message), "$GMCNT,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,*",
            seq_no, 4, 1, 1, 2, 0, 1, 0, 0, 0, 1);

        /* ...then update the checksum... */
        snprintf(strrchr(message, '*') + 1, 5, "%02X\r\n", minmea_checksum(message));

        /* ...and send the message */
        if (fputs(message, fp) == EOF) {
            fprintf(stderr, "Error while writing GMM message to child process\n");
        } else {
            fflush(fp);
        }

        seq_no++;
    }
}

static void generateFakeSCMReadings(int fd)
{
    FILE* fp;
    const float scmReportFreq = 10;
    int dt = 1000 / scmReportFreq;
    /* clang-format off */
    uint8_t message[] = {
        /* Scintillator index 0 */
        0b01100000, 0b10000000, 0b10000000,
        /* 256 empty slots */
        0b01100000, 0b10000010, 0b10000000,
        /* Scintillator index 1 */
        0b01100000, 0b10000000, 0b10000001,
        /* 256 empty slots */
        0b01100000, 0b10000010, 0b10000000,
        /* Scintillator index 2 */
        0b01100000, 0b10000000, 0b10000010,
        /* 256 empty slots */
        0b01100000, 0b10000010, 0b10000000,
        /* End of packet */
        0b01000000, 0b10000000,
    };
    /* clang-format on */

    fp = fdopen(fd, "w");

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(dt));
        if (fwrite(message, sizeof(char), sizeof(message), fp) == EOF) {
            fprintf(stderr, "Error while writing SCM message to child process\n");
        } else {
            fflush(fp);
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

#include "stm32_hal.h"

static osPriority_t convertPriorityToFreeRTOS(task_priority_t prio);
static bool startTask(const task_definition_t* task);

/* Make sure that the new and delete operators use the allocators from FreeRTOS */
void* operator new(std::size_t count)
{
    void* result = pvPortMalloc(count);
    if (result == NULL) {
        throw std::bad_alloc();
    }
    return result;
}

void operator delete(void* ptr) noexcept
{
    vPortFree(ptr);
}

#define HANDLE_FATAL_ERROR(code)                                                \
    {                                                                           \
        /* Store the name of the current task that caused a malloc failed event \
         * so we can report it at the next boot */                              \
        teller::debug::getDebugInfo()->errors |= code;                          \
        memset(teller::debug::getDebugInfo()->task, 0, 16);                     \
        memcpy(teller::debug::getDebugInfo()->task, pcTaskName, 16);            \
        teller::debug::getDebugInfo()->task[15] = 0;                            \
                                                                                \
        teller::hal::notifyFatalError();                                        \
                                                                                \
        taskDISABLE_INTERRUPTS();                                               \
        for (;;)                                                                \
            ;                                                                   \
        /* The watchdog will take care of resetting the board */                \
    }

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
    char* pcTaskName = pcTaskGetName(nullptr);
    HANDLE_FATAL_ERROR(teller::debug::ERROR_MALLOC_FAILED);
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName)
{
    (void)pxTask;
    HANDLE_FATAL_ERROR(teller::debug::ERROR_STACK_OVERFLOW);
}

/* STM32-specific handlers */

extern "C" void HardFault_Handler(void)
{
    char* pcTaskName = pcTaskGetName(nullptr);
    HANDLE_FATAL_ERROR(teller::debug::ERROR_HARD_FAULT);
}

/*
extern "C" void HAL_Delay(uint32_t millis)
{
    vTaskDelay(millis);
}
*/

#endif
