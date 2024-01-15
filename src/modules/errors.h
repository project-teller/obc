#pragma once

namespace teller::errors {

typedef enum {
    NO_ERROR = 0,
    SYSTEM_INIT_ERROR = 1,
    NOT_ENOUGH_MEMORY = 2,
} error_t;

void init();
void setError(error_t code, bool present = true);
void clearError(error_t code);
bool hasError(error_t code);
bool hasAnyErrors();

#define TELLER_CHECK_OOM(ptr)                                            \
    {                                                                    \
        if (ptr == nullptr) {                                            \
            teller::errors::setError(teller::errors::NOT_ENOUGH_MEMORY); \
            return false;                                                \
        }                                                                \
    }

}
