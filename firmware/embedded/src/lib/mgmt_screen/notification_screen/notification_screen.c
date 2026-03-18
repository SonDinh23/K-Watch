#include "notification_screen.h"

#include <stdio.h>
#include <string.h>

#include "../api_screen/fonts/FreeSansBold12pt7b.h"
#include "../api_screen/fonts/FreeSans9pt7b.h"

#define UI_COLOR_WHITE  (0x0E)
#define UI_COLOR_BLACK  (0x00)
#define UI_COLOR_CYAN   (0x06)
#define UI_COLOR_BLUE   (0x02)

static void fit_text_to_width(const GFXfont *font,
			      const char *src,
			      char *dst,
			      size_t dst_len,
			      int16_t max_px)
{
	size_t n;
	size_t cut;

	if (!dst || dst_len == 0U) {
		return;
	}

	if (!src) {
		dst[0] = '\0';
		return;
	}

	n = strlen(src);
	if (kgfx_text_width(font, src) <= max_px) {
		snprintf(dst, dst_len, "%s", src);
		return;
	}

	if (dst_len < 4U) {
		dst[0] = '\0';
		return;
	}

	cut = n;
	while (cut > 0U) {
		size_t copy_len = cut;

		if (copy_len > dst_len - 4U) {
			copy_len = dst_len - 4U;
		}

		memcpy(dst, src, copy_len);
		dst[copy_len] = '.';
		dst[copy_len + 1U] = '.';
		dst[copy_len + 2U] = '.';
		dst[copy_len + 3U] = '\0';

		if (kgfx_text_width(font, dst) <= max_px) {
			return;
		}

		cut--;
	}

	snprintf(dst, dst_len, "...");
}

void notification_screen_render(kgfx_t *g,
				const notification_item_t *items,
				size_t item_count,
				size_t selected_index)
{
	int16_t w;
	int16_t h;
	int16_t y;
	int16_t row_x;
	int16_t row_w;
	int16_t row_h;
	int16_t row_gap;
	int16_t day_col_w;
	int16_t title_y;
	int16_t list_top;
	int16_t list_bottom;
	int16_t available_h;
	const GFXfont *header_font;
	const GFXfont *title_font;
	const GFXfont *preview_font;
	const GFXfont *day_font;
	bool small;
	size_t rows;

	if (!g || !items || item_count == 0U) {
		return;
	}

	w = kgfx_width(g);
	h = kgfx_height(g);
	small = (w <= 128 || h <= 128);

	title_y = small ? 16 : 22;
	row_h = small ? 32 : 38;
	row_gap = small ? 6 : 8;
	day_col_w = small ? 28 : 32;
	row_x = small ? 3 : 4;
	row_w = w - (row_x * 2);
	header_font = small ? &FreeSans9pt7b : &FreeSansBold12pt7b;
	title_font = &FreeSans9pt7b;
	preview_font = &FreeSans9pt7b;
	day_font = &FreeSans9pt7b;
	rows = 3U;
	if (rows > item_count) {
		rows = item_count;
	}

	list_top = small ? 30 : 38;
	list_bottom = small ? 8 : 10;
	available_h = h - list_top - list_bottom;
	if (rows > 0U && available_h > (int16_t)(rows * row_h)) {
		if (rows > 1U) {
			row_gap = (int16_t)((available_h - (int16_t)(rows * row_h)) / (int16_t)(rows - 1U));
			if (row_gap < (small ? 6 : 8)) {
				row_gap = small ? 6 : 8;
			}
		}
	}
	y = list_top + row_h / 2;

	kgfx_fill_screen(g, UI_COLOR_WHITE);

	kgfx_draw_text_aligned(g, header_font,
			       0, title_y, w,
			       KGFX_ALIGN_CENTER, UI_COLOR_BLUE, "Notifications");

	for (size_t i = 0; i < rows; i++) {
		int16_t row_y = y - row_h / 2;
		int16_t body_x = row_x + (small ? 4 : 6);
		int16_t day_x = row_x + row_w - day_col_w;
		int16_t marker = small ? 8 : 10;
		int16_t text_x = body_x + marker + (small ? 5 : 7);
		int16_t text_w = day_x - text_x - (small ? 3 : 4);
		int16_t title_baseline = row_y + (small ? 13 : 15);
		int16_t preview_baseline = row_y + row_h - (small ? 7 : 8);
		char day_text[8];
		char title_text[40];
		char preview_text[48];
		uint8_t row_bg = (i == selected_index) ? UI_COLOR_CYAN : UI_COLOR_WHITE;

		kgfx_fill_round_rect(g, row_x, row_y, row_w, row_h, 3, row_bg);
		kgfx_draw_round_rect(g, row_x, row_y, row_w, row_h, 3, UI_COLOR_BLACK);
		kgfx_draw_fast_vline(g, day_x, row_y + 2, row_h - 4, UI_COLOR_BLACK);

		kgfx_fill_round_rect(g, body_x, row_y + (small ? 10 : 12), marker, marker, 2,
				    UI_COLOR_BLUE);

		fit_text_to_width(title_font, items[i].title,
				  title_text, sizeof(title_text), text_w);
		fit_text_to_width(preview_font, items[i].preview,
				  preview_text, sizeof(preview_text), text_w);

		kgfx_set_font(g, title_font);
		kgfx_set_text_color(g, UI_COLOR_BLACK);
		kgfx_set_cursor(g, text_x, title_baseline);
		kgfx_print(g, title_text);

		kgfx_set_font(g, preview_font);
		kgfx_set_text_color(g, UI_COLOR_BLACK);
		kgfx_set_cursor(g, text_x, preview_baseline);
		kgfx_print(g, preview_text);

		snprintf(day_text, sizeof(day_text), "%ud", (unsigned int)items[i].age_days);
		kgfx_draw_text_aligned(g, day_font,
				       day_x, row_y + row_h / 2 + (small ? 4 : 5),
				       day_col_w, KGFX_ALIGN_CENTER,
				       UI_COLOR_BLUE,
				       day_text);

		y += row_h + row_gap;
	}
}
