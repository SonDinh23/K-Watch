#pragma once

#include <stdint.h>

#include "../api_screen/KGFX.h"

void menu_screen_render(kgfx_t *g,
			const char *title,
			const char *const *items,
			uint8_t item_count,
			uint8_t selected_index);
