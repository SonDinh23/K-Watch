#ifndef FACE_MAIN_H
#define FACE_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "../../driver/LPM013M126A.h"
#include "../../driver/KW_GFX_font.h"
#include "../../driver/KW_GFX.h"

// === include fonts) ===
#include "../../fonts/FreeSansBold24pt7b.h"
#include "../../fonts/FreeSansBold12pt7b.h"
#include "../../fonts/FreeSans9pt7b.h"

void face_main_frame(kw_gfx_t *g);
void face_main_time_update(kw_gfx_t *g, uint8_t hour, uint8_t minute);
void face_main_date_update(kw_gfx_t *g, const char *weekday, const char *date_str);
void face_main_bt_update(kw_gfx_t *g, bool bt_on);
void face_main_notif_update(kw_gfx_t *g, int notif_count);
void face_main_weather_update(kw_gfx_t *g, int temp_now_c, int temp_hi_c, int temp_lo_c);

#endif /* FACE_MAIN_H */