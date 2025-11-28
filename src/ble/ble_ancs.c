#include "ble_ancs.h"

LOG_MODULE_REGISTER(ble_ancs);

static struct bt_ancs_client ancs_c;
static struct bt_gattp gattp;
static atomic_t discovery_flags;

/* Local copy to keep track of the newest arriving notifications. */
static struct bt_ancs_evt_notif notification_latest;
/* Local copy of the newest notification attribute. */
static struct bt_ancs_attr notif_attr_latest;
/* Local copy of the newest app attribute. */
static struct bt_ancs_attr notif_attr_app_id_latest;
/* Buffers to store attribute data. */
static uint8_t attr_appid[ATTR_DATA_SIZE];
static uint8_t attr_title[ATTR_DATA_SIZE];
static uint8_t attr_subtitle[ATTR_DATA_SIZE];
static uint8_t attr_message[ATTR_DATA_SIZE];
static uint8_t attr_message_size[ATTR_DATA_SIZE];
static uint8_t attr_date[ATTR_DATA_SIZE];
static uint8_t attr_posaction[ATTR_DATA_SIZE];
static uint8_t attr_negaction[ATTR_DATA_SIZE];
static uint8_t attr_disp_name[ATTR_DATA_SIZE];

/* String literals for the iOS notification categories.
 * Used then printing to UART.
 */
static const char *lit_catid[BT_ANCS_CATEGORY_ID_COUNT] = {
	"Other",
	"Incoming Call",
	"Missed Call",
	"Voice Mail",
	"Social",
	"Schedule",
	"Email",
	"News",
	"Health And Fitness",
	"Business And Finance",
	"Location",
	"Entertainment"
};

/* String literals for the iOS notification event types.
 * Used then printing to UART.
 */
static const char *lit_eventid[BT_ANCS_EVT_ID_COUNT] = { "Added",
							 "Modified",
							 "Removed" };

/* String literals for the iOS notification attribute types.
 * Used when printing to UART.
 */
static const char *lit_attrid[BT_ANCS_NOTIF_ATTR_COUNT] = {
	"App Identifier",
	"Title",
	"Subtitle",
	"Message",
	"Message Size",
	"Date",
	"Positive Action Label",
	"Negative Action Label"
};


/* === UI aggregator === */
struct ancs_ui_cache {
    uint32_t uid;
    char app_id[ATTR_DATA_SIZE];
    char title [ATTR_DATA_SIZE];
    char msg    [ATTR_DATA_SIZE];
    bool got_app_id, got_title, got_msg;
};
static struct ancs_ui_cache ui_cache;

static void ui_cache_reset(uint32_t uid)
{
    ui_cache.uid = uid;
    ui_cache.app_id[0] = ui_cache.title[0] = ui_cache.msg[0] = '\0';
    ui_cache.got_app_id = ui_cache.got_title = ui_cache.got_msg = false;
}

static inline void copy_attr_str(char *dst, size_t dstsz,
                                 const struct bt_ancs_attr *a)
{
    size_t n = MIN((size_t)a->attr_len, dstsz - 1);
    memcpy(dst, a->attr_data, n);
    dst[n] = '\0';
}

/* === UI work === */
static void ancs_ui_work_handler(struct k_work *work)
{
    /* Bạn đã có test_face_manager() */
    test_face_manager(ui_cache.app_id, ui_cache.title, ui_cache.msg);
}
K_WORK_DEFINE(ancs_ui_work, ancs_ui_work_handler);

static void ancs_try_update_face(void)
{
    /* Cập nhật ngay khi có Title/Message (AppID có hay không vẫn hiển thị được) */
    if (ui_cache.got_title || ui_cache.got_msg) {
        k_work_submit(&ancs_ui_work);
    }
}

/* String literals for the iOS notification attribute types.
 * Used When printing to UART.
 */
static const char *lit_appid[BT_ANCS_APP_ATTR_COUNT] = { "Display Name" };

static void discover_ancs_first(struct bt_conn *conn);
static void discover_ancs_again(struct bt_conn *conn);
static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
		int err, const struct bt_ancs_evt_notif *notif);
static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
		const struct bt_ancs_attr_response *response);
static void bt_ancs_write_response_handler(struct bt_ancs_client *ancs_c,
		uint8_t err);

static void enable_ancs_notifications(struct bt_ancs_client *ancs_c)
{
	int err;

	err = bt_ancs_subscribe_notification_source(ancs_c,
			bt_ancs_notification_source_handler);
	if (err) {
		LOG_ERR("Failed to enable Notification Source notification (err %d)", err);
	}

	err = bt_ancs_subscribe_data_source(ancs_c,
			bt_ancs_data_source_handler);
	if (err) {
		LOG_ERR("Failed to enable Data Source notification (err %d)", err);
	}
}


static void discover_ancs_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_ancs_client *ancs_c = (struct bt_ancs_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	LOG_INF("The discovery procedure for ANCS succeeded");

	bt_gatt_dm_data_print(dm);

	err = bt_ancs_handles_assign(dm, ancs_c);
	if (err) {
		printk("Could not init ANCS client object, error: %d\n", err);
	} else {
		atomic_set_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED);
		enable_ancs_notifications(ancs_c);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}

	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}


static void discover_ancs_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("ANCS could not be found during the discovery\n");

	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static void discover_ancs_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure for ANCS failed, err %d\n", err);

	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static const struct bt_gatt_dm_cb discover_ancs_cb = {
	.completed = discover_ancs_completed_cb,
	.service_not_found = discover_ancs_not_found_cb,
	.error_found = discover_ancs_error_found_cb,
};


static void indicate_sc_cb(struct bt_gattp *gattp,
			   const struct bt_gattp_handle_range *handle_range,
			   int err)
{
	if (!err) {
		atomic_set_bit(&discovery_flags, SERVICE_CHANGED_INDICATED);
		discover_ancs_again(gattp->conn);
	}
}

static void enable_gattp_indications(struct bt_gattp *gattp)
{
	int err;

	err = bt_gattp_subscribe_service_changed(gattp, indicate_sc_cb);
	if (err) {
		printk("Cannot subscribe to Service Changed indication (err %d)\n",
		       err);
	}
}

static void discover_gattp_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_gattp *gattp = (struct bt_gattp *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	/* Checks if the service is empty.
	 * Discovery Manager handles empty services.
	 */
	if (bt_gatt_dm_attr_cnt(dm) > 1) {
		printk("The discovery procedure for GATT Service succeeded\n");

		bt_gatt_dm_data_print(dm);

		err = bt_gattp_handles_assign(dm, gattp);
		if (err) {
			printk("Could not init GATT Service client object, error: %d\n", err);
		} else {
			enable_gattp_indications(gattp);
		}
	} else {
		printk("GATT Service could not be found during the discovery\n");
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}

	discover_ancs_first(conn);
}

static void discover_gattp_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("GATT Service could not be found during the discovery\n");

	discover_ancs_first(conn);
}

static void discover_gattp_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure for GATT Service failed, err %d\n", err);

	discover_ancs_first(conn);
}

static const struct bt_gatt_dm_cb discover_gattp_cb = {
	.completed = discover_gattp_completed_cb,
	.service_not_found = discover_gattp_not_found_cb,
	.error_found = discover_gattp_error_found_cb,
};

static void discover_gattp(struct bt_conn *conn)
{
	int err;

	err = bt_gatt_dm_start(conn, BT_UUID_GATT, &discover_gattp_cb, &gattp);
	if (err) {
		printk("Failed to start discovery for GATT Service (err %d)\n", err);
	}
}

static void discover_ancs(struct bt_conn *conn, bool retry)
{
	int err;

	/* 1 Service Discovery at a time */
	if (atomic_test_and_set_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING)) {
		return;
	}

	/* If ANCS is found, do not discover again. */
	if (atomic_test_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED)) {
		atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
		return;
	}

	/* Check that Service Changed indication is received before discovering ANCS again. */
	if (retry) {
		if (!atomic_test_and_clear_bit(&discovery_flags, SERVICE_CHANGED_INDICATED)) {
			atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
			return;
		}
	}

	err = bt_gatt_dm_start(conn, BT_UUID_ANCS, &discover_ancs_cb, &ancs_c);
	if (err) {
		printk("Failed to start discovery for ANCS (err %d)\n", err);
		atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	}
}

static void discover_ancs_first(struct bt_conn *conn)
{
	discover_ancs(conn, false);
}

static void discover_ancs_again(struct bt_conn *conn)
{
	discover_ancs(conn, true);
}

/**@brief Function for printing an iOS notification.
 *
 * @param[in] notif  Pointer to the iOS notification.
 */
static void notif_print(const struct bt_ancs_evt_notif *notif)
{
	printk("\nNotification\n");
	printk("Event:       %s\n", lit_eventid[notif->evt_id]);
	printk("Category ID: %s\n", lit_catid[notif->category_id]);
	printk("Category Cnt:%u\n", (unsigned int)notif->category_count);
	printk("UID:         %u\n", (unsigned int)notif->notif_uid);

	printk("Flags:\n");
	if (notif->evt_flags.silent) {
		printk(" Silent\n");
	}
	if (notif->evt_flags.important) {
		printk(" Important\n");
	}
	if (notif->evt_flags.pre_existing) {
		printk(" Pre-existing\n");
	}
	if (notif->evt_flags.positive_action) {
		printk(" Positive Action\n");
	}
	if (notif->evt_flags.negative_action) {
		printk(" Negative Action\n");
	}
}


/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS notification attribute.
 */
static void notif_attr_print(const struct bt_ancs_attr *attr)
{
	if (attr->attr_len != 0) {
		printk("%s: %s\n", lit_attrid[attr->attr_id],
		       (char *)attr->attr_data);
	} else if (attr->attr_len == 0) {
		printk("%s: (N/A)\n", lit_attrid[attr->attr_id]);
	}
}

/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS App attribute.
 */
static void app_attr_print(const struct bt_ancs_attr *attr)
{
	if (attr->attr_len != 0) {
		printk("%s: %s\n", lit_appid[attr->attr_id],
		       (char *)attr->attr_data);
	} else if (attr->attr_len == 0) {
		printk("%s: (N/A)\n", lit_appid[attr->attr_id]);
	}
}

/**@brief Function for printing out errors that originated from the Notification Provider (iOS).
 *
 * @param[in] err_code_np Error code received from NP.
 */
static void err_code_print(uint8_t err_code_np)
{
	switch (err_code_np) {
	case BT_ATT_ERR_ANCS_NP_UNKNOWN_COMMAND:
		printk("Error: Command ID was not recognized by the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_INVALID_COMMAND:
		printk("Error: Command failed to be parsed on the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_INVALID_PARAMETER:
		printk("Error: Parameter does not refer to an existing object on the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_ACTION_FAILED:
		printk("Error: Perform Notification Action Failed on the Notification Provider.\n");
		break;

	default:
		break;
	}
}


static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
		int err, const struct bt_ancs_evt_notif *notif)
{
	// if (!err) {
	// 	notification_latest = *notif;
	// 	notif_print(&notification_latest);

	// 	int error = bt_ancs_request_attrs(ancs_c, &notification_latest,
	// 				    bt_ancs_write_response_handler);
	// 	if (error) {
	// 		printk("Failed requesting attributes for a notification (err: %d)\n",
	// 		       error);
	// 	}
	// }

	if (err) return;

    /* Chỉ quan tâm Added/Modified */
    if (notif->evt_id == BT_ANCS_EVENT_ID_NOTIFICATION_REMOVED) {
        return;
    }

    notification_latest = *notif;
    ui_cache_reset(notif->notif_uid);

    int e = bt_ancs_request_attrs(ancs_c, &notification_latest,
                                  bt_ancs_write_response_handler);
    if (e) {
        LOG_ERR("bt_ancs_request_attrs err %d", e);
    }
}

static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
		const struct bt_ancs_attr_response *response)
{
	// switch (response->command_id) {
	// case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES:
	// 	notif_attr_latest = response->attr;
	// 	notif_attr_print(&notif_attr_latest);
	// 	if (response->attr.attr_id ==
	// 	    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER) {
	// 		notif_attr_app_id_latest = response->attr;
	// 	}
	// 	break;

	// case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
	// 	app_attr_print(&response->attr);
	// 	break;

	// default:
	// 	/* No implementation needed. */
	// 	break;
	// }
	if (response->command_id != BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES) {
        if (response->command_id == BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES) {
            /* nếu muốn dùng Display Name của app_id thì copy ở đây */
        }
        return;
    }

    const struct bt_ancs_attr *a = &response->attr;

    switch (a->attr_id) {
    case BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER:
        copy_attr_str(ui_cache.app_id, sizeof(ui_cache.app_id), a);
        ui_cache.got_app_id = true;
        break;

    case BT_ANCS_NOTIF_ATTR_ID_TITLE:
        copy_attr_str(ui_cache.title, sizeof(ui_cache.title), a);
        ui_cache.got_title = true;
        break;

    case BT_ANCS_NOTIF_ATTR_ID_MESSAGE:
        copy_attr_str(ui_cache.msg, sizeof(ui_cache.msg), a);
        ui_cache.got_msg = true;
        break;

    default:
        break;
    }

    /* Khi có đủ (hoặc tối thiểu Title/Message) thì cập nhật UI */
    if (ui_cache.got_title || ui_cache.got_msg) {
        ancs_try_update_face();
    }
}

static void bt_ancs_write_response_handler(struct bt_ancs_client *ancs_c,
					   uint8_t err)
{
	err_code_print(err);
}

static int ancs_c_init(void)
{
	int err;

	err = bt_ancs_client_init(&ancs_c);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER,
		attr_appid, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_app_attr(&ancs_c,
		BT_ANCS_APP_ATTR_ID_DISPLAY_NAME,
		attr_disp_name, sizeof(attr_disp_name));
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_TITLE,
		attr_title, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE,
		attr_message, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_SUBTITLE,
		attr_subtitle, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE,
		attr_message_size, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_DATE,
		attr_date, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL,
		attr_posaction, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL,
		attr_negaction, ATTR_DATA_SIZE);

	return err;
}

static int gattp_init(void)
{
	return bt_gattp_init(&gattp);
}

void bt_ancs_start_discovery(struct bt_conn *conn)
{
    discovery_flags = ATOMIC_INIT(0);
    discover_gattp(conn);
}

int bt_ancs_init(void) {
    int err;
    err = ancs_c_init();
    if (err) {
        LOG_ERR("ANCS client init failed (err %d)", err);
        return err;
    }

    err = gattp_init();
	if (err) {
		LOG_ERR("GATT Service client init failed (err %d)", err);
		return 0;
	}

    return 0;
}
