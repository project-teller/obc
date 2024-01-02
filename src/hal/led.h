#pragma once

namespace teller::hal::led {

typedef enum {
    HEARTBEAT,
    ERROR,
    NUM_LEDS /* sentinel element */
} led_t;

void init(void);
void set(led_t led, bool value = true);
void clear(led_t led);

}
