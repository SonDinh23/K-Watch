#include "ble_peripheral.h"

LOG_MODULE_REGISTER(ble_peripheral, LOG_LEVEL_DBG);

static char nameAdv[CONFIG_BT_DEVICE_NAME_MAX] = "KWATCH";

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0x59, 0x00),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0xC0, 0x00),
    // BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_ANCS_VAL),
};

static struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, nameAdv, sizeof(nameAdv)),
};

static struct bt_le_adv_param adv_params;

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_INF("Updated MTU: TX: %d RX: %d bytes", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Confirm passkey for %s: %06u", addr, passkey);
    
    /* Auto-accept for testing; in production, wait for user confirmation */
    bt_conn_auth_passkey_confirm(conn);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Pairing cancelled: %s", addr);
    bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing failed conn: %s, reason %d %s", addr, reason, bt_security_err_to_str(reason));
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};


static void connected(struct bt_conn *conn, uint8_t err)
{
	int sec_err;
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", addr);

	sec_err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (sec_err) {
		LOG_ERR("Failed to set security (err %d)", sec_err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected from %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);

		if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
			bt_ancs_start_discovery(conn);
		}
	} else {
		LOG_ERR("Security failed: %s level %u err %d %s", addr, level, err, bt_security_err_to_str(err));
	}
}

static void recycled_cb(void)
{
	LOG_INF("Connection object available from previous conn. Disconnect is complete!");
    bt_start_advertise();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
    .recycled = recycled_cb,
};


int bt_begin(void)
{
    int err;

    bt_begin_information();

    err = bt_ancs_init();
    if (err) {
        LOG_ERR("Failed ancs init");
        return err;
    }

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Failed to enable Bluetooth: %d", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");
    return 0;
}

int bt_ready(void)
{
    int err;

    LOG_INF("Bluetooth ready");

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    bt_gatt_cb_register(&gatt_callbacks);

    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks\n");
		return 0;
	}

    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}
    bt_setup_advertise();
    err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising successfully started");
    return 0;
}

int bt_setup_advertise(void)
{
    adv_params.id = BT_ID_DEFAULT;
    adv_params.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
    adv_params.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
    adv_params.options = BT_LE_ADV_OPT_CONN; 
    adv_params.peer = NULL;
    adv_params.secondary_max_skip = 0;
    adv_params.sid = 0;

    return 0;
}

int bt_start_advertise(void)
{
    int err;
    err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }
    return 0;
}

int bt_stop_advertise(void)
{
    return 0;
}