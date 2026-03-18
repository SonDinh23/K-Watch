# Drivers Documentation

This document describes architecture, APIs, integration flow, stability notes, and scaling guidance for all drivers in `src/lib/drivers`.

## 1. Directory Structure

- `src/lib/drivers/CMakeLists.txt`
  - Parent CMake entry for all driver submodules.
- `src/lib/drivers/driver_display_memory/`
  - `LPM013M126A.h/.c`: 176x176 4bpp memory LCD driver.
  - `LS013B7DH03.h/.c`: 128x128 1bpp memory LCD driver.
- `src/lib/drivers/rtc/`
  - `RV8263_C8.h/.c`: RV-8263 RTC driver over I2C.
- `src/lib/drivers/ws2812s/`
  - `ws2812.h/.c`: WS2812 LED strip helper built on Zephyr LED strip API.

## 2. Build Integration

### 2.1 Parent-level integration

`src/lib/drivers/CMakeLists.txt` includes:
- `add_subdirectory(driver_display_memory)`
- `add_subdirectory(rtc)`
- `add_subdirectory(ws2812s)`

Root `CMakeLists.txt` includes only:
- `add_subdirectory(src/lib/drivers)`

This keeps driver build management centralized and scalable.

### 2.2 Submodule-level integration

- `rtc/CMakeLists.txt`: `target_sources(app PRIVATE RV8263_C8.c)`
- `ws2812s/CMakeLists.txt`: `target_sources(app PRIVATE ws2812.c)`
- `driver_display_memory/CMakeLists.txt`: currently builds `LPM013M126A.c` only

## 3. Shared Driver Contract (Current Project Convention)

The project currently follows these practical conventions:

1. Initialization function returns `int` (`0` success, negative errno on failure).
2. Runtime functions may be `void` for high-frequency paths (`draw_pixel`, etc.).
3. Device-tree aliases/labels are used to bind hardware to software.
4. Logging uses Zephyr `LOG_*` and module registration per driver.

## 4. Driver-by-Driver Documentation

## 4.1 RTC Driver (`RV8263_C8`)

### 4.1.1 Purpose
- Read/write date-time from RV-8263 over I2C.
- Support direct init from DT node label `rv8263c8`.

### 4.1.2 Public API
- `int rv8263_init(struct rv8263_dev *dev, const struct i2c_dt_spec *bus)`
- `int rv8263_init_default(struct rv8263_dev *dev)`
- `int rv8263_set_time(struct rv8263_dev *dev, const struct rv8263_time *t)`
- `int rv8263_get_time(struct rv8263_dev *dev, struct rv8263_time *t)`
- `int rv8263_get_epoch(struct rv8263_dev *dev, time_t *epoch_out)`
- `int rv8263_set_time_from_compile(struct rv8263_dev *dev)`

### 4.1.3 Runtime behavior
- On read, if OSC stop flag is set (seconds bit7), returns `-ENODATA`.
- Date/time fields are BCD converted.
- `rv8263_init_default` requires DT label `rv8263c8`.

### 4.1.4 Stability notes
- Good: strict range checks in `rv8263_set_time`.
- Good: `-ENODATA` path allows caller fallback policy.
- Known design choice: compile-time time set uses weekday default (`RV_SUNDAY`), while app-level code may recompute weekday from Y/M/D.

### 4.1.5 Scaling recommendations
- Keep driver generic and policy-free: app decides fallback/retry strategy.
- For multi-board support, add optional init API that accepts node label through Kconfig or DT chosen node.

## 4.2 WS2812 Driver (`ws2812s`)

### 4.2.1 Purpose
- Simple color control helper on top of Zephyr LED strip API.

### 4.2.2 Public API
- `void beginWS2812(void)`
- `void setBrightNess(uint8_t brightness)` (0..100)
- `void setBrightColor(uint8_t r, uint8_t g, uint8_t b)`
- Color shortcuts: `red`, `green`, `blue`, `white`, `notBright`, `purple`, `orange`

### 4.2.3 Runtime behavior
- If `CONFIG_LED_STRIP` is disabled or `DT_ALIAS(led_strip)` is missing/not okay:
  - Driver logs warning.
  - Calls become safe no-op (no crash).
- Brightness scaling is integer and bounded by `CLAMP`.

### 4.2.4 Stability notes
- Good: graceful no-op fallback when strip is unavailable.
- Good: avoids hard compile dependency on custom external utility headers.
- Limitation: API is color-centric only; no animation queue/state machine yet.

### 4.2.5 Scaling recommendations
- Add non-breaking new APIs later:
  - `int ws2812_set_rgb(size_t idx, ...)`
  - `int ws2812_set_all(...)`
  - `int ws2812_flush(void)`
- Keep existing convenience APIs for backward compatibility.

## 4.3 Display Memory Drivers (`driver_display_memory`)

### 4.3.1 Purpose
- Provide common `cmlcd_*` API for different memory LCD panels.

### 4.3.2 Public API surface (both panels)
- `int cmlcd_init(void)`
- `int cmlcd_backlight_set(uint8_t percent)`
- `void cmlcd_draw_pixel(int16_t x, int16_t y, uint8_t color)`
- `void cmlcd_cls(void)`
- `void cmlcd_clear_display(void)`
- `void cmlcd_refresh(void)`
- `void cmlcd_set_trans_mode(uint8_t mode)`
- Blink mode API differs slightly (`void` vs `int`) between implementations.

### 4.3.3 Runtime behavior
- `LPM013M126A`:
  - 4bpp framebuffer.
  - Dirty-line refresh optimization.
  - EXTCOMIN maintained by timer and refresh toggle.
- `LS013B7DH03`:
  - 1bpp framebuffer.
  - Full-frame update sequence.
  - EXTCOMIN toggled during update.

### 4.3.4 Stability notes
- Good: explicit SPI/GPIO readiness checks.
- Good: panel-specific SPI protocol handling is encapsulated.
- Caution: both drivers export same symbol names (`cmlcd_*`), so only one panel source should be linked at a time.

### 4.3.5 Scaling recommendations
- Unify API signatures across panel implementations (including blink mode return type).
- Introduce one thin facade interface (`display_memory_if`) to avoid link-level symbol collision.
- Optionally select implementation by Kconfig/DT in CMake, not by manual source comment toggling.

## 5. Cross-Driver Consistency Review

Current consistency level:
- CMake organization: good after introducing parent `drivers/CMakeLists.txt`.
- Error model: mostly consistent (`int` + errno), but some legacy void APIs remain.
- Naming style: mixed (`snake_case` in RTC/display vs camel-like `beginWS2812`).
- Header hygiene: acceptable, but some headers include heavy Zephyr modules that callers may not need.

Recommended standard for future drivers:
1. File naming: lowercase driver folder, one public header, one source.
2. Public API naming: `module_action` snake_case.
3. Init function returns `int`; operational APIs return `int` where hardware access can fail.
4. Keep board-specific pin/alias policy in DTS; keep driver source policy-light.

## 6. Integration with `main.c`

Current integration pattern:
- Display init -> UI init -> RTC init/read -> periodic refresh loop.
- RTC values update `dashboard_view_model_t`.
- WS2812 color reflects BLE state via helper mapping.

This pattern is stable and easy to extend with additional sensors/actuators.

## 7. Error Code Semantics (Important)

- `-ENODEV`: device or bus unavailable/unready.
- `-EINVAL`: invalid input parameters or parse failure.
- `-ENODATA`: RTC oscillator/time invalid (data not reliable yet).
- `-ENOTSUP`: operation not supported by a specific panel implementation.

## 8. Known Gaps and Next Improvements

1. Display driver CMake selection is still manual for panel implementation.
2. WS2812 API can be expanded for multi-pixel effects.
3. `rv8263_set_time_from_compile` sets weekday to default; app currently compensates by recomputing weekday.
4. Optional: create a `drivers/include/` shared interface headers directory for strict layering.

## 9. Checklist for Adding a New Driver

1. Create folder `src/lib/drivers/<driver_name>/`.
2. Add `<driver_name>.h` and `<driver_name>.c`.
3. Add submodule `CMakeLists.txt` with `target_sources(app PRIVATE ...)`.
4. Add `add_subdirectory(<driver_name>)` into `src/lib/drivers/CMakeLists.txt`.
5. Use `int` + errno returns for init/config functions.
6. Guard all hardware access with device readiness checks.
7. Add logging module and document DT aliases/labels used.
8. Add at least one runtime integration path in app and verify fallback behavior.

## 10. Quick Usage Snippets

### 10.1 RTC

```c
struct rv8263_dev rtc;
struct rv8263_time now;

if (rv8263_init_default(&rtc) == 0 && rv8263_get_time(&rtc, &now) == 0) {
    /* use now */
}
```

### 10.2 WS2812

```c
beginWS2812();
setBrightNess(20);
green();
```

### 10.3 Display Memory

```c
if (cmlcd_init() == 0) {
    cmlcd_draw_pixel(10, 10, LCD_COLOR_WHITE);
    cmlcd_refresh();
}
```

---

If you want, the next step can be an enforcement pass that aligns all driver APIs to one style without breaking the current application entry points.
