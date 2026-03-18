#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../api_screen/KGFX.h"

typedef struct {
	const char *title;
	const char *preview;
	uint16_t age_days;
	bool unread;
} notification_item_t;

void notification_screen_render(kgfx_t *g,
				const notification_item_t *items,
				size_t item_count,
				size_t selected_index);
