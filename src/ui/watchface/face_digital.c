#include "face_digital.h"
#include <stdio.h>

void face_digital_init(kw_gfx_t *g)
{
    /* Initialize with light background */
    kw_gfx_fill_screen(g, LCD_COLOR_WHITE);
    kw_gfx_refresh(g);
}

void face_digital_update(kw_gfx_t *g, uint8_t hour, uint8_t minute, uint8_t second)
{
    (void)second;  /* Unused */
    
    /* Clear screen with black background */
    kw_gfx_fill_screen(g, LCD_COLOR_BLACK);
    
    /* Draw top decorative line (Casio style) */
    kw_gfx_draw_fast_hline(g, 10, 25, 156, LCD_COLOR_WHITE);
    
    /* Draw brand text at top */
    const char *brand = "K-WATCH";
    int brand_width = text_width_gfxfont(&FreeSans9pt7b, brand);
    kw_gfx_draw_text_gfxfont(g, (176 - brand_width) / 2, 20, brand, &FreeSans9pt7b, LCD_COLOR_WHITE);
    
    /* Draw large time HH:MM in center */
    char time_str[8];
    snprintf(time_str, sizeof(time_str), "%02u:%02u", hour, minute);
    
    int time_width = text_width_gfxfont(&FreeSansBold24pt7b, time_str);
    int time_x = (176 - time_width) / 2;
    int time_y = 90;
    
    kw_gfx_draw_text_gfxfont(g, time_x, time_y, time_str, &FreeSansBold24pt7b, LCD_COLOR_WHITE);
    
    /* Draw small seconds below time */
    char sec_str[5];
    snprintf(sec_str, sizeof(sec_str), "%02u", second);
    int sec_width = text_width_gfxfont(&FreeSans12pt7b, sec_str);
    kw_gfx_draw_text_gfxfont(g, (176 - sec_width) / 2, 115, sec_str, &FreeSans12pt7b, LCD_COLOR_WHITE);
    
    /* Draw date/day indicator area (Casio style box) */
    /* Top box border */
    kw_gfx_draw_rect(g, 15, 130, 146, 30, LCD_COLOR_WHITE);
    
    /* Day of week labels */
    const char *day_label = "MON TUE WED THU FRI SAT SUN";
    int day_width = text_width_gfxfont(&Org_01, day_label);
    kw_gfx_draw_text_gfxfont(g, (176 - day_width) / 2, 145, day_label, &Org_01, LCD_COLOR_WHITE);
    
    /* Date placeholder */
    const char *date_str = "28-11";
    int date_width = text_width_gfxfont(&FreeSans9pt7b, date_str);
    kw_gfx_draw_text_gfxfont(g, (176 - date_width) / 2, 155, date_str, &FreeSans9pt7b, LCD_COLOR_WHITE);
    
    /* Bottom decorative elements */
    kw_gfx_draw_fast_hline(g, 10, 168, 156, LCD_COLOR_WHITE);
    
    /* Refresh display */
    kw_gfx_refresh(g);
}

void face_digital_update_with_date(kw_gfx_t *g, uint8_t hour, uint8_t minute, uint8_t second,
                                    uint8_t day, uint8_t month, uint8_t weekday)
{
    /* Light gray background */
    kw_gfx_fill_screen(g, LCD_COLOR_WHITE);
    
    /* Bluetooth icon - top left corner - cleaner design */
    kw_gfx_fill_circle(g, 20, 15, 8, LCD_COLOR_BLUE);
    /* BT letter in white */
    const char *bt = "BT";
    int bt_width = text_width_gfxfont(&Org_01, bt);
    kw_gfx_draw_text_gfxfont(g, 20 - bt_width/2, 18, bt, &Org_01, LCD_COLOR_WHITE);
    
    /* Main rounded card background - white card effect */
    kw_gfx_fill_rect(g, 15, 35, 146, 130, LCD_COLOR_WHITE);
    kw_gfx_draw_rect(g, 15, 35, 146, 130, LCD_COLOR_BLACK);
    
    /* Large time display - HH:MM format */
    char time_str[8];
    snprintf(time_str, sizeof(time_str), "%u:%02u", hour, minute);
    
    int time_width = text_width_gfxfont(&FreeSansBold24pt7b, time_str);
    int time_x = (176 - time_width) / 2;
    int time_y = 80;
    
    kw_gfx_draw_text_gfxfont(g, time_x, time_y, time_str, &FreeSansBold24pt7b, LCD_COLOR_BLACK);
    
    /* Weekday name - centered below time */
    const char *weekdays[] = {"", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    const char *day_name = (weekday >= 1 && weekday <= 7) ? weekdays[weekday] : "---";
    
    int weekday_width = text_width_gfxfont(&FreeSans12pt7b, day_name);
    kw_gfx_draw_text_gfxfont(g, (176 - weekday_width) / 2, 108, day_name, &FreeSans12pt7b, LCD_COLOR_BLACK);
    
    /* Date display - DD Month YYYY format */
    const char *months[] = {"", "January", "February", "March", "April", "May", "June",
                           "July", "August", "September", "October", "November", "December"};
    char date_str[30];
    snprintf(date_str, sizeof(date_str), "%u %s 2025", day, 
             (month >= 1 && month <= 12) ? months[month] : "---");
    
    int date_width = text_width_gfxfont(&FreeSans9pt7b, date_str);
    kw_gfx_draw_text_gfxfont(g, (176 - date_width) / 2, 128, date_str, &FreeSans9pt7b, LCD_COLOR_CYAN);
    
    /* Two icon cards at bottom */
    /* Left card - Mail icon */
    kw_gfx_fill_rect(g, 25, 140, 55, 30, LCD_COLOR_WHITE);
    kw_gfx_draw_rect(g, 25, 140, 55, 30, LCD_COLOR_BLACK);
    
    /* Simple mail envelope icon */
    kw_gfx_draw_rect(g, 38, 150, 28, 18, LCD_COLOR_BLACK);
    kw_gfx_draw_line(g, 38, 150, 52, 159, LCD_COLOR_BLACK);
    kw_gfx_draw_line(g, 66, 150, 52, 159, LCD_COLOR_BLACK);
    
    /* Right card - Notification bell icon with badge */
    kw_gfx_fill_rect(g, 96, 140, 55, 30, LCD_COLOR_WHITE);
    kw_gfx_draw_rect(g, 96, 140, 55, 30, LCD_COLOR_BLACK);
    
    /* Simple bell icon */
    kw_gfx_draw_circle(g, 123, 156, 7, LCD_COLOR_BLACK);
    kw_gfx_draw_line(g, 123, 149, 123, 147, LCD_COLOR_BLACK);
    kw_gfx_draw_fast_hline(g, 119, 163, 8, LCD_COLOR_BLACK);
    
    /* Green notification badge with number */
    kw_gfx_fill_circle(g, 132, 146, 6, LCD_COLOR_GREEN);
    const char *notif = "2";
    int notif_width = text_width_gfxfont(&Org_01, notif);
    kw_gfx_draw_text_gfxfont(g, 132 - notif_width/2, 149, notif, &Org_01, LCD_COLOR_WHITE);
    
    /* Page indicator dots at bottom */
    kw_gfx_fill_circle(g, 78, 170, 2, LCD_COLOR_BLACK);
    kw_gfx_fill_circle(g, 88, 170, 2, LCD_COLOR_CYAN);
    kw_gfx_fill_circle(g, 98, 170, 2, LCD_COLOR_CYAN);
    
    /* Refresh display */
    kw_gfx_refresh(g);
}
