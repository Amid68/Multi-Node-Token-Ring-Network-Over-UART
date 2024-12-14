#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include "token_manager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device *uart_dev;

static void frame_handler(const uint8_t *frame, size_t len)
{
    if (len == 1 && frame[0] == 0xAA) {
        LOG_INF("Received Token Frame");
    } else if (frame[0] == 0xBB) {
        LOG_INF("Received Data Frame len=%d", frame[1]);
        // Print payload as well
        size_t payload_len = frame[1];
        if (payload_len > 0) {
            LOG_INF("Payload: %.*s", payload_len, (char *)&frame[2]);
        }
    }
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    switch (evt->type) {
    case UART_RX_RDY:
        // Feed received data to token manager
        token_manager_feed_data(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
        break;
    case UART_RX_BUF_REQUEST:
        // Provide new buffer if needed, same logic as before
        // (not shown here for brevity, assume you have the double-buffer setup)
        break;
    // Handle other events as before
    default:
        break;
    }
}

void main(void)
{
    uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return;
    }

    // Initialize token manager
    token_manager_init(frame_handler);

    // Set UART callback and enable RX as before
    // ...

    LOG_INF("Ready to receive frames");
}