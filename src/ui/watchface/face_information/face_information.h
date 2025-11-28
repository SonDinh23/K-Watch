#ifndef FACE_INFORMATION_H
#define FACE_INFORMATION_H

#include <stdint.h>
#include <stdbool.h>
#include "../../driver/LPM013M126A.h"
#include "../../driver/KW_GFX_font.h"
#include "../../driver/KW_GFX.h"

// === include fonts) ===
#include "../../fonts/FreeSansBold24pt7b.h"
#include "../../fonts/FreeSansBold12pt7b.h"
#include "../../fonts/FreeSans9pt7b.h"
#include "../../fonts/Roboto_VariableFont_wdth_wght20pt7b.h"

void face_information_frame(kw_gfx_t *g);
void face_information_App_Identifier_update(kw_gfx_t *g, const char *app_id);
void face_information_Title_update(kw_gfx_t *g, const char *title);
void face_information_Notification_update(kw_gfx_t *g, const char *notification);

#endif // FACE_INFORMATION_H