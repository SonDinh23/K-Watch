#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/mgmt_screen/api_screen/KGFX.h"
#include "lib/drivers/rtc/RV8263_C8.h"
#include "lib/mgmt_screen/dashboard_screen/dashboard_screen.h"

int app_runtime_init(dashboard_view_model_t *vm, struct rv8263_dev *rtc, bool *rtc_ready);
void app_runtime_step(dashboard_view_model_t *vm, struct rv8263_dev *rtc, bool *rtc_ready,
		      int64_t uptime_ms);
void app_runtime_render(kgfx_t *g, const dashboard_view_model_t *vm);
bool app_runtime_consume_screen_changed(void);
