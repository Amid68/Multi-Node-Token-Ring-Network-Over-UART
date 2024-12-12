/**
 * @file uart_if.h
 * @brief Platform-agnostic UART Hardware Abstraction Layer (HAL) interface.
 *
 * This header file defines a unified interface for UART initialization,
 * configuration, data transmission, and reception. It abstracts away
 * hardware-specific details, allowing higher-level layers—such as the
 * token management subsystem and application code—to operate without
 * concern for board-specific UART implementations.
 *
 * The API supports:
 * - Non-blocking, interrupt-driven transmission and reception.
 * - Registration of callback functions to handle received data and
 *   transmission completion events.
 * - Configurable parameters (e.g., baud rate, parity) selected at
 *   initialization time.
 *
 * Integration with this interface requires a corresponding implementation
 * in the HAL source file (e.g., uart_hal.c) that interacts with Zephyr’s
 * UART driver model and device tree configurations.
 *
 * @author Ameed Othman
 * @date 12.12.2024
 */

#ifndef __UART_IF_H__
#define __UART_IF_H__

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @typedef uart_hal_callback_t
 * @brief Callback function type for UART events.
 *
 * The callback is invoked when UART reception completes, transmission
 * finishes, or when an asynchronous event (e.g., an error or line status
 * change) occurs. The specific conditions that trigger the callback are
 * defined by the underlying implementation.
 *
 * @param event_data Pointer to event-specific data. Interpretation of 
 *                   this data depends on the event type.
 * @param event_length Length of the event data (in bytes), if applicable.
 */
typedef void (*uart_hal_callback_t)(const uint8_t *event_data, size_t event_length);

/**
 * @brief UART configuration parameters.
 *
 * This structure defines the configuration parameters for UART initialization.
 * Implementations must support these parameters at a minimum, but may expose
 * additional configuration options via Kconfig or prj.conf settings.
 */
struct uart_hal_config {
    uint32_t baud_rate;   /**< Desired UART baud rate (e.g., 115200). */
    uint8_t  data_bits;   /**< Number of data bits (e.g., 8). */
    uint8_t  stop_bits;   /**< Number of stop bits (e.g., 1). */
    uint8_t  parity;      /**< Parity setting (0 = none, 1 = odd, 2 = even). */
    uint8_t  flow_ctrl;   /**< Flow control setting (0 = none, if applicable). */
};

/**
 * @brief Initialize the UART peripheral with specified configuration.
 *
 * This function sets up the UART interface according to the provided
 * configuration parameters. It must be called before any read/write
 * operations. On success, the UART will be ready for non-blocking,
 * interrupt-driven I/O.
 *
 * @param cfg Pointer to a uart_hal_config structure defining UART parameters.
 *
 * @retval 0 if initialization is successful.
 * @retval -EINVAL if invalid configuration parameters are provided.
 * @retval -ENODEV if the UART device cannot be found or initialized.
 */
int uart_hal_init(const struct uart_hal_config *cfg);

/**
 * @brief Register a callback function for UART events.
 *
 * Once registered, the callback will be invoked upon the completion of
 * data reception or transmission, as well as other asynchronous UART events.
 * Providing a NULL pointer will deregister the current callback.
 *
 * @param cb Pointer to the callback function. NULL to deregister.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if asynchronous callbacks are not supported.
 */
int uart_hal_set_callback(uart_hal_callback_t cb);

/**
 * @brief Write data to the UART transmit buffer.
 *
 * This function queues data for transmission in a non-blocking manner.
 * Once the data is sent, the registered callback (if any) will be invoked
 * to indicate completion. If no callback is registered, the caller may
 * poll for completion using a separate mechanism.
 *
 * @param data Pointer to the data buffer to transmit.
 * @param length Number of bytes to transmit.
 *
 * @retval Number of bytes successfully queued for transmission.
 * @retval -EINVAL if the data pointer is NULL or length is zero.
 * @retval -EIO if a hardware or driver error prevents queuing the data.
 */
int uart_hal_write(const uint8_t *data, size_t length);

/**
 * @brief Read data from the UART receive buffer.
 *
 * This function retrieves available data from the UART receive buffer. If
 * no data is currently available, it returns 0. For asynchronous reception,
 * the callback notifies when new data arrives. Synchronous readers can
 * poll this function periodically.
 *
 * @param data Pointer to a buffer where received data will be stored.
 * @param length Maximum number of bytes to read.
 *
 * @retval Number of bytes read. If 0, no data is currently available.
 * @retval -EINVAL if data pointer is NULL or length is zero.
 * @retval -EIO if a hardware or driver error occurs during read.
 */
int uart_hal_read(uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
