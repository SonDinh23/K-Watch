#include "../../watchface/face_information/face_information.h"

void face_information_frame(kw_gfx_t *g) {
    kw_gfx_fill_screen(g, LCD_COLOR_WHITE); // white background
    kw_gfx_fill_rect(g, 0, 40, 176, 3, LCD_COLOR_BLACK); // black panel
    kw_gfx_fill_rect(g, 0, 90, 176, 3, LCD_COLOR_BLACK); // black panel
}

void face_information_App_Identifier_update(kw_gfx_t *g, const char *app_id) {
    kw_gfx_draw_text_gfxfont(g, 5, 45, app_id, &Roboto_VariableFont_wdth_wght20pt7b, LCD_COLOR_BLACK);
}

void face_information_Title_update(kw_gfx_t *g, const char *title) {
    kw_gfx_draw_text_gfxfont(g, 5, 95, title, &Roboto_VariableFont_wdth_wght20pt7b, LCD_COLOR_BLACK);
}

void face_information_Notification_update(kw_gfx_t *g, const char *notification) {
    kw_gfx_draw_text_gfxfont(g, 5, 130, notification, &Roboto_VariableFont_wdth_wght20pt7b, LCD_COLOR_BLACK);
}

// void face_main_frame(kw_gfx_t *g) {
//     kw_gfx_fill_screen(g, LCD_COLOR_BLACK); // black background
//     kw_gfx_fill_rect(g, 0, 30, 176, 90, LCD_COLOR_WHITE); // white panel
// }

// void face_main_time_update(kw_gfx_t *g, uint8_t hour, uint8_t minute) {
//     char time_str[8];
//     snprintf(time_str, sizeof(time_str), "%02u:%02u", hour, minute);
//     int w_time = text_width_gfxfont(&FreeSansBold24pt7b, time_str);
//     int x_time = (176 - w_time) / 2;
//     int y_time = 78;  
//     kw_gfx_draw_text_gfxfont(g, x_time, y_time, time_str, &FreeSansBold24pt7b, LCD_COLOR_BLACK);
// }

// void face_main_date_update(kw_gfx_t *g, const char *weekday, const char *date_str) {
//     char date_line[32];
//     snprintf(date_line, sizeof(date_line), "%s  %s", weekday, date_str);
//     int w_date = text_width_gfxfont(&FreeSans9pt7b, date_line);
//     kw_gfx_draw_text_gfxfont(g, (176 - w_date)/2, 105, date_line, &FreeSans9pt7b, LCD_COLOR_BLACK);
// }