#pragma once

#include <cstdint>

namespace teller::debug {

/**
 * @brief Flags correspnding to various error conditions that we want to log
 * in the debugging data structure to survive a reboot.
 */
typedef enum {
    ERROR_STACK_OVERFLOW = 1,
    ERROR_MALLOC_FAILED = 2,
    ERROR_HARD_FAULT = 4,
} internal_error_t;

/**
 * @brief Common blink patterns to use on the heartbeat LED.
 */
typedef enum {
    BLINK_OFF = 0,
    BLINK_HEARTBEAT = 0b10100000,
    BLINK_FAST = 0b10101010,
    BLINK_MEDIUM = 0b11001100,
    BLINK_SLOW = 0b11110000,
    BLINK_SOLID = 0b11111111,
} blink_pattern_t;

/**
 * @brief Struct used to store debugging information that should survive a
 * soft reboot.
 */
typedef struct {
    /**
     * Identifies whether this is the first boot. If this is not the
     * first boot, it should contain 0xC99C9CC9
     */
    uint32_t first_boot_marker;

    /**
     * Flags indicating internal errors that have occurred.
     */
    uint32_t errors;

    /** Name of the task that caused a stack overflow in the last boot */
    char task[16];
} debug_info_t;

/**
 * @brief Initializes the debugging module.
 *
 * This function must be called early during the boot process.
 */
void init(void);

/**
 * @brief Destroys the debugging module.
 */
void destroy(void);

/**
 * @brief Returns the current blink pattern to show on the heartbeat LED.
 */
uint8_t getBlinkPattern(void);

/**
 * @brief Returns a pointer to the debug data structure.
 */
debug_info_t* getDebugInfo(void);

/**
 * @brief Returns the error flags that are set in the debug structure and clears them.
 */
uint32_t getAndClearErrorFlags(void);

/**
 * @brief Reports any errors that were stored in the error flags.
 *
 * This function should be called during the boot process at the time when the
 * telemetry streams are up and running.
 */
void reportErrorsDuringPreviousBoot(void);

/**
 * @brief Sets the current blink pattern to show on the heartbeat LED.
 */
void setBlinkPattern(uint8_t pattern);

}
