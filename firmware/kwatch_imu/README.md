# K_Watch IMU (LSM6DSL) Demo

> Minimal Zephyr example demonstrating the LSM6DSL IMU (accelerometer + gyroscope)

## Summary

This repository contains a compact Zephyr application that demonstrates reading accelerometer
and gyroscope data from an ST LSM6DSL sensor. The firmware configures the sensor, optionally
registers an interrupt trigger, and prints sensor samples to the console (RTT or serial) at a
regular interval.

This project is intended as a small, reliable validation/demo to verify wiring, DeviceTree
configuration and driver operation on Zephyr-supported boards.

## Highlights

- Small, focused demo that uses Zephyr's sensor API.
- Configures accelerometer and gyroscope sampling frequency to 104 Hz.
- Demonstrates both polled sampling and optional `DATA_READY` trigger handling.
- Uses RTT logging by default (see `prj.conf`).

## Requirements

- Zephyr SDK and toolchain installed and configured (see Zephyr documentation).
- `west` or CMake/Ninja build tools.
- A Zephyr-supported target board with the LSM6DSL or an appropriate sensor connected.
- LSM6DSL device tree node or board overlay that makes the driver available to the build.

## Project Structure

- `src/main.c` — Main application: initializes the sensor, sets sampling frequency, optionally
	registers a trigger handler, and prints samples.
- `CMakeLists.txt` — Zephyr project entry; adds `src/main.c` to the `app` target.
- `prj.conf` — Zephyr configuration (RTT logging, sensor, I2C/SPI, trigger thread, etc.).

## Quick Start — Build & Flash

Using `west` (recommended):

```bash
west build -b <board_name> -s . -d build
west flash
```

Using CMake/Ninja directly:

```bash
mkdir -p build
cd build
cmake -DBOARD=<board_name> -S .. -B .
ninja -C .
# To flash: ninja -C . flash (or use your board's flashing tool)
```

Replace `<board_name>` with your target board name.

## Run & Verify

- Attach a serial console (e.g. 115200 8N1) or RTT to observe logs. This project enables RTT
	logging by default (`CONFIG_USE_SEGGER_RTT`, `CONFIG_RTT_CONSOLE` in `prj.conf`).
- On startup the application acquires the LSM6DSL device using `DEVICE_DT_GET_ONE(st_lsm6dsl)`,
	sets accel/gyro sampling frequency to 104 Hz, optionally registers a `SENSOR_TRIG_DATA_READY`
	handler, then prints sensor values every 2 seconds.

Example output (abbreviated):

```
LSM6DSL sensor samples:

accel x:0.000000 ms/2 y:0.000000 ms/2 z:9.810000 ms/2
gyro x:0.000000 dps y:0.000000 dps z:0.000000 dps
loop:1 trig_cnt:0
```

Notes:
- The code prints `loop` count and `trig_cnt` (number of trigger events handled).
- If the trigger is enabled via `CONFIG_LSM6DSL_TRIGGER`, the handler reads accel/gyro and
	increments `lsm6dsl_trig_cnt` each event.

## DeviceTree / Board Overlay

- Ensure your board has an LSM6DSL node or attach the sensor and provide a board overlay that
	enables the driver. The code expects a device compatible with `st,lsm6dsl` (it uses
	`DEVICE_DT_GET_ONE(st_lsm6dsl)`).

Example overlay snippet (adapt to your board and bus):

```dts
&i2c1 {
		lsm6dsl@6a {
				compatible = "st,lsm6dsl";
				reg = <0x6a>;
				label = "LSM6DSL";
		};
};

/ {
		chosen {
				zephyr,sensor = &lsm6dsl@6a;
		};
};
```

Adjust the node, bus and address to match your wiring.

## Troubleshooting & Notes

- `device_is_ready()` failed: verify the DeviceTree node is present and the driver is enabled.
- Console: this project uses RTT by default. If you prefer UART, update `prj.conf` (`CONFIG_UART_CONSOLE=y`
	and disable RTT settings if needed).
- External sensors (magnetometer, pressure/temp) are conditionally read if present and
	enabled in the build configuration.
- Performance: the example clears and updates the whole printed state each loop; for
	production use consider event-driven sampling or streaming data off-board.

## Contributing

PRs and issues welcome. Helpful contributions include board overlays, wiring diagrams,
tests, or example apps that consume IMU data.

## License

This project follows the SPDX header used in `src/main.c` (Apache-2.0).

---

If you want, I can also:
- Commit this `README.md` for you.
- Add a sample board overlay (`prj.overlay`) for your board — tell me the board name and bus.
- Add a short example to stream sensor data over RTT or a minimal consumer example.

