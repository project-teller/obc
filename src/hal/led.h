#pragma once

namespace teller::hal::led {

typedef enum {
    HEARTBEAT,
    ERROR,
    NUM_LEDS /* sentinel element */
} led_t;

/**
 * @brief Initialization function for the LED subsystem.
 *
 * This function is called from the global HAL initialization function at
 * startup time.
 */
void init(void);

void set(led_t led, bool value = true);
void clear(led_t led);

}
