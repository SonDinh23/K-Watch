#ifndef FACE_MANAGER_H
#define FACE_MANAGER_H

#include <stdint.h>
#include "face_information/face_information.h"
#include "face_digital.h"

void face_manager_init(void);
void face_manager_show_digital(uint8_t hour, uint8_t minute, uint8_t second);
void face_manager_show_digital_with_date(uint8_t hour, uint8_t minute, uint8_t second,
                                          uint8_t day, uint8_t month, uint8_t weekday);
void test_face_manager(const char *app_id, const char *title, const char *notification);

#endif // FACE_MANAGER_H