#ifndef BLE_PERIPHERAL_H
#define BLE_PERIPHERAL_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "../ble/ble_information.h"
#include "../ble/ble_ancs.h"

#ifdef __cplusplus
extern "C" {
#endif

int bt_begin(void);
int bt_ready(void);

int bt_setup_advertise(void);
int bt_start_advertise(void);
int bt_stop_advertise(void);

// all of your legacy C code here

#ifdef __cplusplus
}
#endif

#endif // BLE_PERIPHERAL_H