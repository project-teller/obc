#pragma once

#include <sstream>

#include "hal/uart.h"

namespace teller::hal::uart {

/**
 * @brief Redirects the output of a UART to a string buffer.
 *
 * The class follows the RAII pattern -- the redirection comes into effect when
 * the class is instantiated and the original state is restored when the class
 * instance is destroyed.
 */
class UARTOutputRedirector {

public:
    UARTOutputRedirector(uart_t index);
    ~UARTOutputRedirector();

    UARTOutputRedirector(const UARTOutputRedirector&) = delete;
    UARTOutputRedirector& operator=(const UARTOutputRedirector&) = delete;

    /**
     * @brief Returns the contents of the internal buffer of the redirector.
     */
    std::string get() const;

    /**
     * @brief Returns and clears the contents of the internal buffer of the redirector.
     */
    std::string getAndClear();

private:
    uart_t _index;
};

/**
 * @brief Feeds the input of a UART from a string buffer.
 *
 * The class follows the RAII pattern -- the override comes into effect when
 * the class is instantiated and the original state is restored when the class
 * instance is destroyed.
 */
class UARTInputRedirector {

public:
    UARTInputRedirector(uart_t index);
    ~UARTInputRedirector();

    UARTInputRedirector(const UARTInputRedirector&) = delete;
    UARTInputRedirector& operator=(const UARTInputRedirector&) = delete;

    /**
     * @brief Clears any pending data that has not been sent to the redirector yet.
     */
    void clear();

    /**
     * @brief Feeds new data into the redirector.
     */
    void feed(const std::string& value);

private:
    uart_t _index;
};

}
