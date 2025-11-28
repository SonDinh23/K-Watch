#include "ble_information.h"

LOG_MODULE_REGISTER(ble_information, LOG_LEVEL_DBG);

#define VND_MAX_LEN 20

static uint8_t vnd_value[VND_MAX_LEN + 1] = { 'V', 'e', 'n', 'd', 'o', 'r'};

void bt_begin_information()
{
	LOG_INF("Test function called");
}

static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > VND_MAX_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	value[offset + len] = 0;

	return len;
}

BT_GATT_SERVICE_DEFINE(ble_information,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_vnd, write_vnd, vnd_value),
	// BT_GATT_CCC(vnd_ccc_cfg_changed,
	// 	    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	// BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid,                                                                             
	// 		       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
	// 		       BT_GATT_PERM_READ_AUTHEN |
	// 		       BT_GATT_PERM_WRITE_AUTHEN,
	// 		       read_vnd, write_vnd, vnd_auth_value),
	// BT_GATT_CHARACTERISTIC(&vnd_long_uuid.uuid, BT_GATT_CHRC_READ |
	// 		       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
	// 		       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
	// 		       BT_GATT_PERM_PREPARE_WRITE,
	// 		       read_vnd, write_long_vnd, &vnd_long_value),
	// BT_GATT_CEP(&vnd_long_cep),
	// BT_GATT_CHARACTERISTIC(&vnd_signed_uuid.uuid, BT_GATT_CHRC_READ |
	// 		       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_AUTH,
	// 		       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
	// 		       read_signed, write_signed, &signed_value),
	// BT_GATT_CHARACTERISTIC(&vnd_write_cmd_uuid.uuid,
	// 		       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
	// 		       BT_GATT_PERM_WRITE, NULL,
	// 		       write_without_rsp_vnd, &vnd_wwr_value),
);