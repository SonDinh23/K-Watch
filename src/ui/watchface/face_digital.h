#ifndef FACE_DIGITAL_H
#define FACE_DIGITAL_H

#include <stdint.h>
#include "../driver/LPM013M126A.h"
#include "../driver/KW_GFX_font.h"
#include "../driver/KW_GFX.h"
#include "../fonts/FreeSansBold24pt7b.h"
#include "../fonts/FreeSansBold12pt7b.h"
#include "../fonts/FreeSans12pt7b.h"
#include "../fonts/FreeSans9pt7b.h"
#include "../fonts/Org_01.h"

/**
 * Initialize digital watchface
 */
void face_digital_init(kw_gfx_t *g);

/**
 * Update digital watchface with time
 * @param g Graphics context
 * @param hour Hours (0-23)
 * @param minute Minutes (0-59)
 * @param second Seconds (0-59)
 */
void face_digital_update(kw_gfx_t *g, uint8_t hour, uint8_t minute, uint8_t second);

/**
 * Update digital watchface with date
 * @param g Graphics context
 * @param hour Hours (0-23)
 * @param minute Minutes (0-59)
 * @param second Seconds (0-59)
 * @param day Day (1-31)
 * @param month Month (1-12)
 * @param weekday Weekday (1-7, 1=Monday)
 */
void face_digital_update_with_date(kw_gfx_t *g, uint8_t hour, uint8_t minute, uint8_t second,
                                    uint8_t day, uint8_t month, uint8_t weekday);

#endif /* FACE_DIGITAL_H */
