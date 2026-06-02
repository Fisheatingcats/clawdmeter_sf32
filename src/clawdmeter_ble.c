#include "clawdmeter_ble.h"
#include "clawdmeter_data.h"

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdio.h>

#ifdef CONFIG_BLUETOOTH

#include "bf0_ble_gap.h"
#include "bf0_sibles.h"
#include "bf0_sibles_advertising.h"
#include "ble_connection_manager.h"

#define LOG_TAG "cm_ble"
#include "log.h"

/* ════════════════════════════════════════════════════════════════════
 *  UUID definitions (little-endian byte arrays)
 *  Service:  6c415a55-4465-7669-6365-000000000001
 *  RX chr:   6c415a55-4465-7669-6365-000000000002  (write)
 *  TX chr:   6c415a55-4465-7669-6365-000000000003  (read + notify)
 *  Req chr:  6c415a55-4465-7669-6365-000000000004  (notify)
 * ════════════════════════════════════════════════════════════════════ */

enum cm_att_list {
    CM_SVC,
    CM_RX_CHAR,
    CM_RX_VALUE,
    CM_TX_CHAR,
    CM_TX_VALUE,
    CM_TX_CCCD,
    CM_REQ_CHAR,
    CM_REQ_VALUE,
    CM_REQ_CCCD,
    CM_ATT_NB
};

static uint8_t svc_uuid[ATT_UUID_128_LEN] = {
    0x01,0x00,0x00,0x00, 0x00,0x00,0x65,0x63,
    0x69,0x76,0x65,0x44, 0x55,0x5a,0x41,0x6c
};
static uint8_t rx_uuid[ATT_UUID_128_LEN] = {
    0x02,0x00,0x00,0x00, 0x00,0x00,0x65,0x63,
    0x69,0x76,0x65,0x44, 0x55,0x5a,0x41,0x6c
};
static uint8_t tx_uuid[ATT_UUID_128_LEN] = {
    0x03,0x00,0x00,0x00, 0x00,0x00,0x65,0x63,
    0x69,0x76,0x65,0x44, 0x55,0x5a,0x41,0x6c
};
static uint8_t req_uuid[ATT_UUID_128_LEN] = {
    0x04,0x00,0x00,0x00, 0x00,0x00,0x65,0x63,
    0x69,0x76,0x65,0x44, 0x55,0x5a,0x41,0x6c
};

#define SERIAL_UUID_16(x) {((uint8_t)(x&0xff)),((uint8_t)(x>>8))}

BLE_GATT_SERVICE_DEFINE_128(cm_att_db)
{
    /* Service declaration */
    BLE_GATT_SERVICE_DECLARE(CM_SVC, SERIAL_UUID_16_PRI_SERVICE,
                             BLE_GATT_PERM_READ_ENABLE),

    /* ── RX characteristic (write from host) ── */
    BLE_GATT_CHAR_DECLARE(CM_RX_CHAR, SERIAL_UUID_16_CHARACTERISTIC,
                          BLE_GATT_PERM_READ_ENABLE),
    BLE_GATT_CHAR_VALUE_DECLARE(CM_RX_VALUE, rx_uuid,
                                BLE_GATT_PERM_WRITE_REQ_ENABLE |
                                BLE_GATT_PERM_WRITE_COMMAND_ENABLE,
                                BLE_GATT_VALUE_PERM_UUID_128 |
                                BLE_GATT_VALUE_PERM_RI_ENABLE,
                                CM_JSON_MAX_LEN),

    /* ── TX characteristic (read + notify to host) ── */
    BLE_GATT_CHAR_DECLARE(CM_TX_CHAR, SERIAL_UUID_16_CHARACTERISTIC,
                          BLE_GATT_PERM_READ_ENABLE),
    BLE_GATT_CHAR_VALUE_DECLARE(CM_TX_VALUE, tx_uuid,
                                BLE_GATT_PERM_READ_ENABLE |
                                BLE_GATT_PERM_NOTIFY_ENABLE,
                                BLE_GATT_VALUE_PERM_UUID_128 |
                                BLE_GATT_VALUE_PERM_RI_ENABLE,
                                256),
    BLE_GATT_DESCRIPTOR_DECLARE(CM_TX_CCCD, SERIAL_UUID_16_CLIENT_CHAR_CFG,
                                BLE_GATT_PERM_READ_ENABLE |
                                BLE_GATT_PERM_WRITE_REQ_ENABLE,
                                BLE_GATT_VALUE_PERM_RI_ENABLE, 2),

    /* ── Request characteristic (notify to host) ── */
    BLE_GATT_CHAR_DECLARE(CM_REQ_CHAR, SERIAL_UUID_16_CHARACTERISTIC,
                          BLE_GATT_PERM_READ_ENABLE),
    BLE_GATT_CHAR_VALUE_DECLARE(CM_REQ_VALUE, req_uuid,
                                BLE_GATT_PERM_READ_ENABLE |
                                BLE_GATT_PERM_NOTIFY_ENABLE,
                                BLE_GATT_VALUE_PERM_UUID_128 |
                                BLE_GATT_VALUE_PERM_RI_ENABLE,
                                4),
    BLE_GATT_DESCRIPTOR_DECLARE(CM_REQ_CCCD, SERIAL_UUID_16_CLIENT_CHAR_CFG,
                                BLE_GATT_PERM_READ_ENABLE |
                                BLE_GATT_PERM_WRITE_REQ_ENABLE,
                                BLE_GATT_VALUE_PERM_RI_ENABLE, 2),
};

/* ════════════════════════════════════════════════════════════════════
 *  State
 * ════════════════════════════════════════════════════════════════════ */

static sibles_hdl s_srv_handle;
static uint8_t    s_conn_idx;
static uint8_t    s_tx_config;   /* CCCD value for TX */
static uint8_t    s_req_config;  /* CCCD value for Request */
static char       s_last_tx[24] = "{}";
static bool       s_initialized;
static char       s_dev_name[32] = "Clawdmeter";
static char       s_address[20] = "";  /* "XX:XX:XX:XX:XX:XX" */

static rt_mailbox_t s_ble_mb;

/* ════════════════════════════════════════════════════════════════════
 *  GATT callbacks
 * ════════════════════════════════════════════════════════════════════ */

static uint8_t *cm_gatts_get_cbk(uint8_t conn_idx, uint8_t idx, uint16_t *len)
{
    (void)conn_idx;
    uint8_t *ret = NULL;
    *len = 0;

    switch (idx) {
    case CM_TX_VALUE:
        ret = (uint8_t *)s_last_tx;
        *len = (uint16_t)strlen(s_last_tx);
        break;
    case CM_REQ_VALUE:
        /* Minimal value for read */
        ret = (uint8_t *)"\x01";
        *len = 1;
        break;
    default:
        break;
    }
    return ret;
}

static uint8_t cm_gatts_set_cbk(uint8_t conn_idx, sibles_set_cbk_t *para)
{
    s_conn_idx = conn_idx;

    switch (para->idx) {
    case CM_RX_VALUE: {
        /* Host wrote JSON data */
        if (para->len == 0 || para->len > CM_JSON_MAX_LEN) {
            snprintf(s_last_tx, sizeof(s_last_tx), "{\"err\":true}");
            break;
        }
        char buf[CM_JSON_MAX_LEN];
        memcpy(buf, para->value, para->len);
        buf[para->len] = '\0';

        int rc = cm_data_update_json(buf, para->len);
        if (rc == 0) {
            snprintf(s_last_tx, sizeof(s_last_tx), "{\"ack\":true}");
        } else {
            LOG_W("JSON rejected");
            snprintf(s_last_tx, sizeof(s_last_tx), "{\"err\":true}");
        }
        /* Send TX notification */
        if (s_tx_config) {
            sibles_value_t v;
            v.hdl  = s_srv_handle;
            v.idx  = CM_TX_VALUE;
            v.len  = (uint16_t)strlen(s_last_tx);
            v.value = (uint8_t *)s_last_tx;
            sibles_write_value(s_conn_idx, &v);
        }
        break;
    }
    case CM_TX_CCCD:
        s_tx_config = *(para->value);
        LOG_I("TX CCCD: %d", s_tx_config);
        break;
    case CM_REQ_CCCD:
        s_req_config = *(para->value);
        LOG_I("REQ CCCD: %d", s_req_config);
        /* If no data yet, request refresh */
        if (s_req_config) {
            cm_usage_data_t d;
            if (!cm_data_get(&d)) {
                cm_ble_request_refresh();
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

/* ════════════════════════════════════════════════════════════════════
 *  BLE event handler
 * ════════════════════════════════════════════════════════════════════ */

static int cm_ble_event_handler(uint16_t event_id, uint8_t *data,
                                uint16_t len, uint32_t context)
{
    switch (event_id) {
    case BLE_POWER_ON_IND:
        if (s_ble_mb)
            rt_mb_send(s_ble_mb, (rt_uint32_t)BLE_POWER_ON_IND);
        break;

    case BLE_GAP_CONNECTED_IND: {
        ble_gap_connect_ind_t *ind = (ble_gap_connect_ind_t *)data;
        s_conn_idx = ind->conn_idx;
        LOG_I("BLE connected (idx=%d)", s_conn_idx);
        break;
    }
    case BLE_GAP_DISCONNECTED_IND:
        LOG_I("BLE disconnected");
        s_tx_config  = 0;
        s_req_config = 0;
        break;

    case SIBLES_MTU_EXCHANGE_IND: {
        sibles_mtu_exchange_ind_t *ind = (sibles_mtu_exchange_ind_t *)data;
        LOG_I("MTU exchanged: %d", ind->mtu);
        break;
    }
    default:
        break;
    }
    return 0;
}
BLE_EVENT_REGISTER(cm_ble_event_handler, NULL);

/* ════════════════════════════════════════════════════════════════════
 *  Service init + advertising (called after BLE_POWER_ON_IND)
 * ════════════════════════════════════════════════════════════════════ */

static void cm_ble_register_service(void)
{
    BLE_GATT_SERVICE_INIT_128(svc, cm_att_db, CM_ATT_NB,
                              BLE_GATT_SERVICE_PERM_NOAUTH |
                              BLE_GATT_SERVICE_PERM_UUID_128 |
                              BLE_GATT_SERVICE_PERM_MULTI_LINK,
                              svc_uuid);

    s_srv_handle = sibles_register_svc_128(&svc);
    if (s_srv_handle)
        sibles_register_cbk(s_srv_handle, cm_gatts_get_cbk, cm_gatts_set_cbk);
    LOG_I("GATT service registered, hdl=%d", s_srv_handle);
}

SIBLES_ADVERTISING_CONTEXT_DECLAR(cm_adv_ctx);

static uint8_t cm_adv_event(uint8_t event, void *context, void *data)
{
    switch (event) {
    case SIBLES_ADV_EVT_ADV_STARTED: {
        sibles_adv_evt_startted_t *evt = (sibles_adv_evt_startted_t *)data;
        LOG_I("ADV started, status=%d mode=%d", evt->status, evt->adv_mode);
        break;
    }
    case SIBLES_ADV_EVT_ADV_STOPPED: {
        sibles_adv_evt_stopped_t *evt = (sibles_adv_evt_stopped_t *)data;
        LOG_I("ADV stopped, reason=%d", evt->reason);
        break;
    }
    default:
        break;
    }
    return 0;
}

static void cm_ble_start_advertising(void)
{
    sibles_advertising_para_t para = {0};

    /* Get and format MAC address */
    bd_addr_t addr;
    if (ble_get_public_address(&addr) == HL_ERR_NO_ERROR) {
        rt_snprintf(s_address, sizeof(s_address), "%02X:%02X:%02X:%02X:%02X:%02X",
                    addr.addr[0], addr.addr[1], addr.addr[2],
                    addr.addr[3], addr.addr[4], addr.addr[5]);
    }

    /* Build device name with MAC suffix for uniqueness */
    char local_name[32];
    if (s_address[0]) {
        rt_snprintf(local_name, sizeof(local_name), "Clawdmeter-%02X%02X",
                    addr.addr[4], addr.addr[5]);
    } else {
        memcpy(local_name, "Clawdmeter", 11);
    }
    memcpy(s_dev_name, local_name, sizeof(s_dev_name));

    /* Set device name */
    ble_gap_dev_name_t *dn = rt_malloc(sizeof(ble_gap_dev_name_t) + strlen(s_dev_name));
    dn->len = (uint8_t)strlen(s_dev_name);
    memcpy(dn->name, s_dev_name, dn->len);
    ble_gap_set_dev_name(dn);
    rt_free(dn);

    LOG_I("BLE name: %s, addr: %s", s_dev_name, s_address);

    para.own_addr_type = GAPM_STATIC_ADDR;
    para.config.adv_mode = SIBLES_ADV_CONNECT_MODE;
    para.config.mode_config.conn_config.duration = 0;  /* forever */
    para.config.mode_config.conn_config.interval = 0x30; /* 30ms */
    para.config.max_tx_pwr = 0x7F;
    para.config.is_auto_restart = 1;

    /* Name in scan response */
    para.rsp_data.completed_name = rt_malloc(strlen(s_dev_name) + sizeof(sibles_adv_type_name_t));
    para.rsp_data.completed_name->name_len = (uint8_t)strlen(s_dev_name);
    memcpy(para.rsp_data.completed_name->name, s_dev_name, strlen(s_dev_name));

    para.evt_handler = cm_adv_event;

    uint8_t ret = sibles_advertising_init(cm_adv_ctx, &para);
    if (ret == SIBLES_ADV_NO_ERR)
        sibles_advertising_start(cm_adv_ctx);

    rt_free(para.rsp_data.completed_name);
}

/* ════════════════════════════════════════════════════════════════════
 *  Public API
 * ════════════════════════════════════════════════════════════════════ */

int cm_ble_init(void)
{
    if (s_initialized) return 0;

    s_ble_mb = rt_mb_create("cm_ble", 8, RT_IPC_FLAG_FIFO);
    if (!s_ble_mb) return -1;

    /* Enable BLE stack */
    sifli_ble_enable();
    s_initialized = true;
    return 0;
}

int cm_ble_send_json(const char *json)
{
    if (!s_tx_config || !json) return -1;

    snprintf(s_last_tx, sizeof(s_last_tx), "%s", json);

    sibles_value_t v;
    v.hdl   = s_srv_handle;
    v.idx   = CM_TX_VALUE;
    v.len   = (uint16_t)strlen(s_last_tx);
    v.value = (uint8_t *)s_last_tx;
    int ret = sibles_write_value(s_conn_idx, &v);
    return (ret == v.len) ? 0 : -1;
}

void cm_ble_request_refresh(void)
{
    if (!s_req_config) return;

    uint8_t val = 1;
    sibles_value_t v;
    v.hdl   = s_srv_handle;
    v.idx   = CM_REQ_VALUE;
    v.len   = 1;
    v.value = &val;
    sibles_write_value(s_conn_idx, &v);
}

bool cm_ble_is_connected(void)
{
    return s_tx_config != 0;
}

const char *cm_ble_device_name(void)
{
    return s_dev_name;
}

const char *cm_ble_address(void)
{
    return s_address[0] ? s_address : "--:--:--:--:--:--";
}

/* ════════════════════════════════════════════════════════════════════
 *  BLE mailbox polling (called from main loop or dedicated thread)
 * ════════════════════════════════════════════════════════════════════ */

/**
 * Process BLE events from the mailbox. Call this periodically
 * or after cm_ble_init() returns.
 */
void cm_ble_poll(void)
{
    if (!s_ble_mb) return;

    rt_uint32_t msg;
    while (rt_mb_recv(s_ble_mb, &msg, RT_WAITING_NO) == RT_EOK) {
        if (msg == BLE_POWER_ON_IND) {
            LOG_I("BLE power on");
            cm_ble_register_service();
            cm_ble_start_advertising();
        }
    }
}

#ifndef NVDS_AUTO_UPDATE_MAC_ADDRESS_ENABLE
ble_common_update_type_t ble_request_public_address(bd_addr_t *addr)
{
    int ret = bt_mac_addr_generate_via_uid_v2(addr);
    if (ret != 0) {
        LOG_W("generate mac address failed %d", ret);
        return BLE_UPDATE_NO_UPDATE;
    }
    return BLE_UPDATE_ONCE;
}
#endif

#else /* !CONFIG_BLUETOOTH */

/* ── Stubs when BLE is not enabled ── */

int  cm_ble_init(void)          { return 0; }
int  cm_ble_send_json(const char *json) { (void)json; return -1; }
void cm_ble_request_refresh(void) {}
bool cm_ble_is_connected(void)  { return false; }
const char *cm_ble_device_name(void) { return "Clawdmeter"; }
const char *cm_ble_address(void) { return "--:--:--:--:--:--"; }
void cm_ble_poll(void) {}

#endif /* CONFIG_BLUETOOTH */
