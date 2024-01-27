#include <cassert>

#include "config.h"
#include "hal/led.h"

using namespace std;
using namespace teller::hal::led;

static bool led_state[NUM_LEDS];
static void clearAllLEDs();

void teller::hal::led::init()
{
    clearAllLEDs();
}

void teller::hal::led::destroy()
{
    clearAllLEDs();
}

void teller::hal::led::clear(led_t led)
{
    set(led, false);
}

bool teller::hal::led::get(led_t led)
{
    assert(led >= 0 && led < NUM_LEDS);
    return led_state[led];
}

void teller::hal::led::set(led_t led, bool value)
{
    assert(led >= 0 && led < NUM_LEDS);
    led_state[led] = value;
}

/* ************************************************************************** */

static void clearAllLEDs()
{
    for (size_t i = 0; i < NUM_LEDS; i++) {
        led_state[i] = false;
    }
}
