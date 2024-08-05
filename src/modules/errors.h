#pragma once

namespace teller::errors {

typedef enum {
    NO_ERROR = 0,
    SYSTEM_INIT_ERROR = 1,
    NOT_ENOUGH_MEMORY = 2,
    QUEUE_FULL = 3,
} error_t;

void init(void);
void destroy(void);

/**
 * @brief Marks the error condition with the given error code as present.
 *
 * @param code  the error code
 * @param present  whether the error should be marked as present or absent
 */
void setError(error_t code, bool present = true);

/**
 * @brief Clears the error condition with the given error code.
 *
 * This is equivalent to calling \ref "setError()" with \c present = \c false .
 *
 * @param code  the error code
 */
void clearError(error_t code);

/**
 * @brief Clears all error conditions.
 */
void clearAllErrors(void);

/**
 * @brief Returns whether the error condition with the given code is present.
 *
 * @param code  the error code
 * @return true if the error condition is present, false otherwise
 */
bool hasError(error_t code);

/**
 * @brief Returns whether any error conditions are marked as present.
 * @return true if at least one error condition is present, false otherwise
 */
bool hasAnyErrors(void);

/**
 * @brief Returns the code of a single arbitrary error condition that is present.
 *
 * There are no guarantees about \em which error code will be returned if multiple
 * error conditions are present. However, it is guaranteed that the return value
 * will be the same if the set of present error conditions does not change.
 *
 * @return the code of a single arbitrary error condition that is present, or
 *         zero if no error conditions are present
 */
error_t getError(void);

#define TELLER_CHECK_OOM(ptr)                                            \
    {                                                                    \
        if (ptr == nullptr) {                                            \
            /* GCOVR_EXCL_START */                                       \
            teller::errors::setError(teller::errors::NOT_ENOUGH_MEMORY); \
            return false;                                                \
            /* GCOVR_EXCL_STOP */                                        \
        }                                                                \
    }

}
