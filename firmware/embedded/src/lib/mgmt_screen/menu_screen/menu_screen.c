#include "menu_screen.h"

#include <stddef.h>

#include "../api_screen/fonts/FreeSans9pt7b.h"
#include "../api_screen/fonts/FreeSansBold12pt7b.h"

#define UI_COLOR_WHITE  (0x0E)
#define UI_COLOR_BLACK  (0x00)
#define UI_COLOR_CYAN   (0x06)

void menu_screen_render(kgfx_t *g,
			const char *title,
			const char *const *items,
			uint8_t item_count,
			uint8_t selected_index)
{
	int16_t w;
	int16_t y;

	if (!g || !title || !items || item_count == 0U) {
		return;
	}

	w = kgfx_width(g);
	kgfx_fill_screen(g, UI_COLOR_WHITE);

	kgfx_set_font(g, &FreeSansBold12pt7b);
	kgfx_set_text_color(g, UI_COLOR_BLACK);
	kgfx_draw_text_aligned(g, &FreeSansBold12pt7b, 0, 20, w,
			       KGFX_ALIGN_CENTER, UI_COLOR_BLACK, title);

	y = 44;
	for (uint8_t i = 0; i < item_count; i++) {
		uint8_t row_bg = (i == selected_index) ? UI_COLOR_CYAN : UI_COLOR_WHITE;
		uint8_t row_fg = UI_COLOR_BLACK;
		int16_t row_h = 28;
		int16_t row_x = 8;
		int16_t row_w = w - 16;

		kgfx_fill_round_rect(g, row_x, y - 16, row_w, row_h, 3, row_bg);
		kgfx_draw_round_rect(g, row_x, y - 16, row_w, row_h, 3, UI_COLOR_BLACK);

		kgfx_set_font(g, &FreeSans9pt7b);
		kgfx_set_text_color(g, row_fg);
		kgfx_set_cursor(g, row_x + 10, y);
		kgfx_print(g, items[i]);

		if (i == selected_index) {
			kgfx_set_cursor(g, row_x + row_w - 14, y);
			kgfx_print(g, ">");
		}

		y += 32;
	}
}
