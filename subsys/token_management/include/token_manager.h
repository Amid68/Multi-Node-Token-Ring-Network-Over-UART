#ifndef TOKEN_MANAGER_H_
#define TOKEN_MANAGER_H_

#include <zephyr/types.h>
#include <stddef.h>

/* Callback type for notifying when a complete frame is parsed */
typedef void (*frame_callback_t)(const uint8_t *frame, size_t len);

void token_manager_init(frame_callback_t cb);
void token_manager_feed_data(const uint8_t *data, size_t len);

#endif /* TOKEN_MANAGER_H_ */