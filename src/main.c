#include "uart_if.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_test, LOG_LEVEL_DBG);

static void uart_test_callback(const uint8_t *data, size_t length)
{
    if (data && length > 0) {
        LOG_INF("Received %u bytes: %.*s", length, length, data);
    } else {
        LOG_INF("TX complete or an event occurred with no data");
    }
}

void main(void)
{
    struct uart_hal_config cfg = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
        .flow_ctrl = 0,
    };

    if (uart_hal_init(&cfg) != 0) {
        LOG_ERR("UART HAL init failed");
        return;
    }

    uart_hal_set_callback(uart_test_callback);

    while (1) {
        const char *msg = "TEST\n";
        int written = uart_hal_write((const uint8_t *)msg, strlen(msg));
        if (written <= 0) {
            LOG_WRN("Failed to write data");
        }
        k_sleep(K_MSEC(1000));
    }
}