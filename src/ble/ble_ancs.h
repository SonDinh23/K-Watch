#ifndef BLE_ANCS_H
#define BLE_ANCS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ancs_client.h>
#include <bluetooth/services/gattp.h>

#include <zephyr/settings/settings.h>

#include "../ui/watchface/face_manager.h"

/* Allocated size for attribute data. */
#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

enum {
	DISCOVERY_ANCS_ONGOING,
	DISCOVERY_ANCS_SUCCEEDED,
	SERVICE_CHANGED_INDICATED
};

void bt_ancs_start_discovery(struct bt_conn *conn);
int bt_ancs_init(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_ANCS_H