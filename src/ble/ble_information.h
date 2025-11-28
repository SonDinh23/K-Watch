#ifndef BLE_INFORMATION_H
#define BLE_INFORMATION_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>


#ifdef __cplusplus
extern "C" {
#endif

void bt_begin_information();

#ifdef __cplusplus
}
#endif

#endif // BLE_INFORMATION_H