#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "token_manager.h"

LOG_MODULE_REGISTER(token_manager, LOG_LEVEL_INF);

static frame_callback_t user_cb = NULL;

/* Internal buffer for accumulating incoming data.
In a real scenario, you might want a ring buffer or a dynamic approach.
For simplicity, use a static buffer and handle overflows gracefully. */
#define PARSE_BUF_SIZE 256
static uint8_t parse_buf[PARSE_BUF_SIZE];
static size_t parse_buf_len = 0;

/* Simple parser states */
enum parse_state {
    PARSE_STATE_IDLE,
    PARSE_STATE_DATA_LEN,
    PARSE_STATE_DATA_PAYLOAD
};

static enum parse_state state = PARSE_STATE_IDLE;
static uint8_t expected_len = 0;
static size_t payload_received = 0;

static void reset_parser(void)
{
    parse_buf_len = 0;
    state = PARSE_STATE_IDLE;
    expected_len = 0;
    payload_received = 0;
}

/* Attempt to parse frames from parse_buf.
This function is called after each data feed to check if we have complete frames. */
static void try_parse_frames(void)
{
    size_t i = 0;
    while (i < parse_buf_len) {
    uint8_t b = parse_buf[i];
           switch (state) {
       case PARSE_STATE_IDLE:
           if (b == 0xAA) {
               /* Token frame detected */
               if (user_cb) {
                   uint8_t frame = 0xAA;
                   user_cb(&frame, 1);
               }
               i++;
           } else if (b == 0xBB) {
               /* Start of data frame */
               state = PARSE_STATE_DATA_LEN;
               i++;
           } else {
               /* Unknown byte at idle state, consume it and move on */
               i++;
           }
           break;
       case PARSE_STATE_DATA_LEN:
           /* Next byte is length */
           expected_len = b;
           payload_received = 0;
           state = PARSE_STATE_DATA_PAYLOAD;
           i++;
           break;
       case PARSE_STATE_DATA_PAYLOAD:
           /* We have started reading payload bytes. Need expected_len payload + 1 checksum. */
           {
               size_t bytes_available = parse_buf_len - i;
               size_t total_needed = expected_len + 1; // payload + checksum
               if (bytes_available >= total_needed - payload_received) {
                   /* We have all needed data */
                   size_t needed_now = total_needed - payload_received;
                   // frame format: 0xBB, length, payload..., checksum
                   // frame start index is (i - 2) because we read 0xBB and length already
                   // Actually, we know 0xBB was at (i - 2), length at (i - 1)
                   size_t frame_start = i - 2; // 0xBB at frame_start, length at frame_start+1
                   size_t frame_len = 2 + total_needed; // BB, length, payload, checksum
                   // Extract the full frame
                   uint8_t frame[256]; // Enough buffer
                   for (size_t x = 0; x < frame_len; x++) {
                       frame[x] = parse_buf[frame_start + x];
                   }

                   // Call user callback
                   if (user_cb) {
                       user_cb(frame, frame_len);
                   }

                   i = frame_start + frame_len;
                   reset_parser();
               } else {
                   /* Not enough data yet, consume what we have and wait for more */
                   payload_received += bytes_available;
                   i = parse_buf_len; // consume all available
               }
           }
           break;
       }
   }

   /* After trying to parse, shift any unparsed data to the front */
   if (i < parse_buf_len) {
       size_t remaining = parse_buf_len - i;
       memmove(parse_buf, parse_buf + i, remaining);
       parse_buf_len = remaining;
   } else {
       parse_buf_len = 0;
   }
}

void token_manager_init(frame_callback_t cb)
{
    user_cb = cb;
    reset_parser();
}

void token_manager_feed_data(const uint8_t *data, size_t len)
{
    if (len > (PARSE_BUF_SIZE - parse_buf_len)) {
    LOG_WRN(“Parse buffer overflow, discarding data”);
    return;
    }

   memcpy(parse_buf + parse_buf_len, data, len);
   parse_buf_len += len;

   try_parse_frames();
}
