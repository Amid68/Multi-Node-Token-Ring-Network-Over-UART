/**
 * @file uart_hal.c
 * @brief UART Hardware Abstraction Layer (HAL) implementation.
 *
 * This source file provides the implementation of the platform-agnostic UART HAL
 * interface defined in uart_if.h. It leverages Zephyr’s UART API to configure
 * UART peripherals, set up interrupt-driven (asynchronous) communication, and
 * manage data transfers through ring buffers.
 *
 * The design goals for this implementation include:
 * - Hardware Independence: The code relies on device tree bindings and board
 *   overlays to locate and configure the appropriate UART peripheral for the
 *   target board, ensuring the same codebase works across multiple platforms.
 * - Non-Blocking Operation: Both transmission and reception are performed
 *   asynchronously. The calling application can register a callback to be
 *   notified when data is received or when transmissions complete, freeing
 *   the main application from continuous polling.
 * - Robustness & Professional Standards: Thorough parameter checks, meaningful
 *   return codes, and comprehensive comments ensure that this implementation
 *   is maintainable, scalable, and easy to integrate.
 *
 * To adapt this code for a particular board or custom requirements:
 * - Verify that the device tree overlays in boards/arm/ match your hardware setup.
 * - Update prj.conf or Kconfig files if you need to enable/disable certain UART
 *   features (e.g., hardware flow control, alternate UART instances).
 *
 * This file assumes that the Zephyr asynchronous UART API is enabled (e.g.,
 * CONFIG_UART_ASYNC_API=y). Refer to Zephyr’s official documentation for more
 * details on UART driver configuration and capabilities.
 *
 * Note: This is a reference implementation and may be refined further to handle
 * more complex scenarios, such as dynamic reconfiguration, multiple UART instances,
 * or advanced error handling strategies.
 *
 * @author Ameed Othman
 * @date 12.12.2024
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/uart.h>
#include <sys/ring_buffer.h>
#include <logging/log.h>
#include "uart_if.h"

LOG_MODULE_REGISTER(uart_hal, LOG_LEVEL_INF);

/*---------------------------------------------------------------------------*/
/*                          Configuration and Defines                         */
/*---------------------------------------------------------------------------*/

#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256

/*
 * Adjust these buffer sizes as needed. Larger buffers can handle more data
 * at the cost of increased RAM usage. Smaller buffers are more memory-efficient
 * but may increase the risk of overruns if the application or ISR does not
 * consume/produce data quickly enough.
 */

/*---------------------------------------------------------------------------*/
/*                          Static Variables                                  */
/*---------------------------------------------------------------------------*/

/* UART Device Binding */
static const struct device *uart_dev = NULL;

/* User-Registered Callback */
static uart_hal_callback_t user_callback = NULL;

/* Receive and Transmit Ring Buffers */
static uint8_t rx_ring_buf_mem[UART_RX_BUFFER_SIZE];
static uint8_t tx_ring_buf_mem[UART_TX_BUFFER_SIZE];

static struct ring_buf rx_ring_buf;
static struct ring_buf tx_ring_buf;

/* Mutexes for protecting ring buffer operations */
K_MUTEX_DEFINE(rx_mutex);
K_MUTEX_DEFINE(tx_mutex);

/* Active TX Tracking */
static volatile bool tx_in_progress = false;

/*---------------------------------------------------------------------------*/
/*                          Utility Functions                                 */
/*---------------------------------------------------------------------------*/

/**
 * @brief Invoke the user callback if registered.
 *
 * @param data Pointer to event-specific data
 * @param length Length of event-specific data
 */
static inline void invoke_user_callback(const uint8_t *data, size_t length)
{
    if (user_callback) {
        user_callback(data, length);
    }
}

/*---------------------------------------------------------------------------*/
/*                          UART Callback Handler                             */
/*---------------------------------------------------------------------------*/

static void uart_event_handler(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    switch (evt->type) {
    case UART_RX_RDY:
        /* Data received successfully */
        if (evt->data.rx.len > 0) {
            k_mutex_lock(&rx_mutex, K_FOREVER);
            /* Push received data into RX ring buffer */
            uint32_t written = ring_buf_put(&rx_ring_buf, evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
            k_mutex_unlock(&rx_mutex);

            if (written < evt->data.rx.len) {
                LOG_WRN("RX ring buffer overflow, some data lost");
            }

            /* Notify user of received data */
            invoke_user_callback(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
        }
        break;

    case UART_RX_BUF_REQUEST:
        /* If using double-buffering, could provide a new buffer here. For simplicity,
           rely on automatic buffer management. */
        break;

    case UART_RX_BUF_RELEASED:
        /* A previously used RX buffer was released back to driver. Nothing special
           to do here unless custom buffer management is implemented. */
        break;

    case UART_TX_DONE:
        /* Transmission completed for the given chunk of data */
        k_mutex_lock(&tx_mutex, K_FOREVER);

        /* Transmission done: check if more data is pending in TX ring buffer */
        uint8_t temp_buf[64];
        uint32_t bytes_to_send = ring_buf_get(&tx_ring_buf, temp_buf, sizeof(temp_buf));

        if (bytes_to_send > 0) {
            /* Continue transmission with remaining data */
            int ret = uart_tx(uart_dev, temp_buf, bytes_to_send, SYS_FOREVER_MS);
            if (ret < 0) {
                LOG_ERR("Failed to continue TX: %d", ret);
                tx_in_progress = false;
            }
        } else {
            /* No more data to send */
            tx_in_progress = false;
        }
        k_mutex_unlock(&tx_mutex);

        /* Notify user that a portion of data has been transmitted */
        invoke_user_callback(NULL, 0);
        break;

    case UART_TX_ABORTED:
        /* Transmission aborted (e.g., due to error or stop request) */
        LOG_WRN("UART transmission aborted");
        tx_in_progress = false;
        invoke_user_callback(NULL, 0); /* Notify user if needed */
        break;

    case UART_RX_STOPPED:
        /* Reception stopped due to an error */
        LOG_ERR("UART reception stopped due to error: %d", evt->data.rx_stop.reason);
        invoke_user_callback(NULL, 0); /* Could pass error details if desired */
        break;

    default:
        LOG_DBG("Unhandled UART event: %d", evt->type);
        break;
    }
}

/*---------------------------------------------------------------------------*/
/*                          Public Functions (HAL)                           */
/*---------------------------------------------------------------------------*/

int uart_hal_init(const struct uart_hal_config *cfg)
{
    if (!cfg) {
        return -EINVAL;
    }

    /* Get UART device from device tree.
       By default, we might rely on chosen node "zephyr,console" or a specific alias.
       Adjust as needed based on project requirements. */
    uart_dev = device_get_binding(DT_LABEL(DT_CHOSEN(zephyr_console)));
    if (!uart_dev) {
        LOG_ERR("Failed to find UART device");
        return -ENODEV;
    }

    /* Configure UART parameters. For asynchronous API in Zephyr, these parameters
       might need to be set via Kconfig/device tree. For demonstration, we assume
       basic defaults are acceptable. If runtime configuration is required, use
       uart_line_ctrl_set() calls here (e.g., UART_LINE_CTRL_BAUD_RATE). */

    int ret = uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_BAUD_RATE, cfg->baud_rate);
    if (ret < 0) {
        LOG_ERR("Failed to set baud rate: %d", ret);
        return ret;
    }

    uint32_t data_bits = cfg->data_bits;
    ret = uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DATA_BITS, data_bits);
    if (ret < 0) {
        LOG_ERR("Failed to set data bits: %d", ret);
        return ret;
    }

    uint32_t stop_bits = cfg->stop_bits;
    ret = uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_STOP_BITS, stop_bits);
    if (ret < 0) {
        LOG_ERR("Failed to set stop bits: %d", ret);
        return ret;
    }

    uint32_t parity = cfg->parity;
    ret = uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_PARITY, parity);
    if (ret < 0) {
        LOG_ERR("Failed to set parity: %d", ret);
        return ret;
    }

    /* Flow control may or may not be supported. If supported, similar line ctrl calls can be made. */

    /* Initialize ring buffers */
    ring_buf_init(&rx_ring_buf, sizeof(rx_ring_buf_mem), rx_ring_buf_mem);
    ring_buf_init(&tx_ring_buf, sizeof(tx_ring_buf_mem), tx_ring_buf_mem);

    /* Set up asynchronous UART callbacks */
    struct uart_config zcfg = {
        .baudrate = cfg->baud_rate,
        .data_bits = cfg->data_bits,
        .stop_bits = cfg->stop_bits,
        .parity = cfg->parity,
        .flow_ctrl = cfg->flow_ctrl
    };

    ret = uart_configure(uart_dev, &zcfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure UART: %d", ret);
        return ret;
    }

    ret = uart_callback_set(uart_dev, uart_event_handler, NULL);
    if (ret < 0) {
        LOG_ERR("Failed to set UART callback: %d", ret);
        return ret;
    }

    /* Start RX if async API needs an explicit call. Some drivers require a buffer to be provided. */
    static uint8_t initial_rx_buf[64];
    ret = uart_rx_enable(uart_dev, initial_rx_buf, sizeof(initial_rx_buf), SYS_FOREVER_MS);
    if (ret < 0) {
        LOG_ERR("Failed to enable UART RX: %d", ret);
        return ret;
    }

    LOG_INF("UART HAL initialized with baud=%d, data_bits=%d, stop_bits=%d, parity=%d",
            cfg->baud_rate, cfg->data_bits, cfg->stop_bits, cfg->parity);

    return 0;
}

int uart_hal_set_callback(uart_hal_callback_t cb)
{
    user_callback = cb;
    /* If no callback is supported by lower levels, return -ENOTSUP, but here we rely on async API. */
    return 0;
}

int uart_hal_write(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return -EINVAL;
    }

    k_mutex_lock(&tx_mutex, K_FOREVER);
    uint32_t written = ring_buf_put(&tx_ring_buf, data, length);

    if (written == 0) {
        k_mutex_unlock(&tx_mutex);
        LOG_WRN("TX buffer full, cannot enqueue data");
        return -EIO;
    }

    /* If no transmission in progress, start one now */
    if (!tx_in_progress) {
        uint8_t temp_buf[64];
        uint32_t bytes_to_send = ring_buf_get(&tx_ring_buf, temp_buf, sizeof(temp_buf));
        if (bytes_to_send > 0) {
            int ret = uart_tx(uart_dev, temp_buf, bytes_to_send, SYS_FOREVER_MS);
            if (ret == 0) {
                tx_in_progress = true;
            } else {
                LOG_ERR("Failed to start TX: %d", ret);
                k_mutex_unlock(&tx_mutex);
                return -EIO;
            }
        }
    }
    k_mutex_unlock(&tx_mutex);

    /* Return how many bytes we managed to enqueue */
    return (int)written;
}

int uart_hal_read(uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return -EINVAL;
    }

    k_mutex_lock(&rx_mutex, K_FOREVER);
    uint32_t read_len = ring_buf_get(&rx_ring_buf, data, length);
    k_mutex_unlock(&rx_mutex);

    /* If read_len = 0, no data currently available */
    return (int)read_len;
}
