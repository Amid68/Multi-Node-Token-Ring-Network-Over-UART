#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static uint8_t rx_buf[2][64];
static int current_buf = 0;

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    switch (evt->type) {
    case UART_RX_RDY:
        LOG_INF("Received %d bytes: %.*s", evt->data.rx.len, evt->data.rx.len,
                evt->data.rx.buf + evt->data.rx.offset);
        break;
    case UART_RX_BUF_REQUEST:
        /* Provide a new buffer when requested */
        current_buf = (current_buf + 1) % 2;
        uart_rx_buf_rsp(dev, rx_buf[current_buf], sizeof(rx_buf[current_buf]));
        break;
    case UART_RX_BUF_RELEASED:
        /* The previously used RX buffer is released here. Nothing special needed if double-buffering. */
        break;
    case UART_RX_DISABLED:
        LOG_WRN("RX disabled");
        break;
    case UART_TX_DONE:
        LOG_INF("TX complete");
        break;
    case UART_TX_ABORTED:
        LOG_ERR("TX aborted");
        break;
    default:
        LOG_DBG("Unhandled UART event: %d", evt->type);
        break;
    }
}

void main(void)
{
    const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return;
    }

    uart_callback_set(uart_dev, uart_cb, NULL);

    /* Enable RX with the first buffer */
    int ret = uart_rx_enable(uart_dev, rx_buf[0], sizeof(rx_buf[0]), SYS_FOREVER_MS);
    if (ret < 0) {
        LOG_ERR("Failed to enable UART RX: %d", ret);
        return;
    }

    const char *msg = "Hello UART!\n";
    for (const char *p = msg; *p != '\0'; p++) {
        uart_poll_out(uart_dev, *p);
    }

    LOG_INF("Message sent over UART");
}