#include <stdint.h>
#include <string.h>

#include "ble.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_gatts.h"

#include "mible_api.h"
#include "mi_config.h"

#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "nRF"
#include "mible_log.h"

static uint8_t m_gap_users, m_gattc_users, m_gatts_users, m_arch_users;
static mible_gap_callback_t   m_gap_cb_table[MIBLE_MAX_USERS];
static mible_gatts_callback_t m_gatts_cb_table[MIBLE_MAX_USERS];
static mible_gattc_callback_t m_gattc_cb_table[MIBLE_MAX_USERS];
static mible_arch_callback_t  m_arch_cb_table[MIBLE_MAX_USERS];

static void gap_evt_dispatch(ble_evt_t *p_ble_evt)
{
	static uint16_t conn_handle = BLE_CONN_HANDLE_INVALID;
	uint8_t gap_evt_availble = 0;
	mible_gap_evt_t evt;
	mible_gap_evt_param_t gap_params = {0};
	
	switch(p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		evt = MIBLE_GAP_EVT_CONNECTED;
		conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		gap_params.conn_handle = conn_handle;
		
        gap_params.connect.type = p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr_type == BLE_GAP_ADDR_TYPE_PUBLIC ? MIBLE_ADDRESS_TYPE_PUBLIC : MIBLE_ADDRESS_TYPE_RANDOM;
		memcpy(gap_params.connect.peer_addr, p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr, 6);;
		
		gap_params.connect.conn_param.max_conn_interval = p_ble_evt->evt.gap_evt.params.connected.conn_params.max_conn_interval;
		gap_params.connect.conn_param.min_conn_interval = p_ble_evt->evt.gap_evt.params.connected.conn_params.min_conn_interval;
		gap_params.connect.conn_param.slave_latency     = p_ble_evt->evt.gap_evt.params.connected.conn_params.slave_latency;
		gap_params.connect.conn_param.conn_sup_timeout  = p_ble_evt->evt.gap_evt.params.connected.conn_params.conn_sup_timeout;

        gap_params.connect.role = p_ble_evt->evt.gap_evt.params.connected.role == BLE_GAP_ROLE_PERIPH ? MIBLE_GAP_PERIPHERAL : MIBLE_GAP_CENTRAL;

		gap_evt_availble = 1;
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		evt = MIBLE_GAP_EVT_DISCONNET;
		conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		gap_params.conn_handle = conn_handle;
		gap_params.disconnect.reason      = p_ble_evt->evt.gap_evt.params.disconnected.reason;
		gap_evt_availble = 1;
		break;

	case BLE_GAP_EVT_CONN_PARAM_UPDATE:
		evt = MIBLE_GAP_EVT_CONN_PARAM_UPDATED;
		gap_params.conn_handle = conn_handle;
		gap_params.update_conn.conn_param.max_conn_interval = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval;
		gap_params.update_conn.conn_param.min_conn_interval = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.min_conn_interval;
		gap_params.update_conn.conn_param.slave_latency     = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.slave_latency;
		gap_params.update_conn.conn_param.conn_sup_timeout  = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout;

		gap_evt_availble = 1;
		break;

	case BLE_GAP_EVT_ADV_REPORT:
		evt = MIBLE_GAP_EVT_ADV_REPORT;
		ble_gap_evt_adv_report_t adv = p_ble_evt->evt.gap_evt.params.adv_report;
		gap_params.conn_handle = conn_handle;
		gap_params.report.addr_type = adv.peer_addr.addr_type == BLE_GAP_ADDR_TYPE_PUBLIC ? MIBLE_ADDRESS_TYPE_PUBLIC : MIBLE_ADDRESS_TYPE_RANDOM;
		memcpy(gap_params.report.peer_addr, adv.peer_addr.addr, 6);
		gap_params.report.adv_type = adv.scan_rsp ? SCAN_RSP_DATA : ADV_DATA;
		gap_params.report.rssi = adv.rssi;
		memcpy(gap_params.report.data, adv.data, adv.dlen);;
		gap_params.report.data_len = adv.dlen;
		gap_evt_availble = 1;
		break;
	}
	
	if (gap_evt_availble)
		mible_gap_event_callback(evt, &gap_params) ;
	
}

static void gatts_evt_dispatch(ble_evt_t *p_ble_evt)
{
	uint32_t errno;
	mible_gatts_evt_t evt;
	mible_gatts_evt_param_t gatts_params = {0};
	gatts_params.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
    ble_gatts_value_t gatts_value = {0};
	ble_gatts_evt_rw_authorize_request_t rw_req = {0};
    ble_gatts_rw_authorize_reply_params_t reply = {0};

	switch(p_ble_evt->header.evt_id) {
	case BLE_GATTS_EVT_WRITE:
		gatts_params.write.value_handle = p_ble_evt->evt.gatts_evt.params.write.handle;
		gatts_params.write.data = p_ble_evt->evt.gatts_evt.params.write.data;
		gatts_params.write.len = p_ble_evt->evt.gatts_evt.params.write.len;
		gatts_params.write.offset = p_ble_evt->evt.gatts_evt.params.write.offset;
		if (p_ble_evt->evt.gatts_evt.params.write.auth_required)
			evt = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
		else
			evt = MIBLE_GATTS_EVT_WRITE;

		mible_gatts_event_callback(evt, &gatts_params);
		break;
		
	case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
		rw_req = p_ble_evt->evt.gatts_evt.params.authorize_request;
		if (p_ble_evt->evt.gatts_evt.params.authorize_request.type == BLE_GATTS_AUTHORIZE_TYPE_READ) {
			gatts_params.read.value_handle = rw_req.request.read.handle;
			evt = MIBLE_GATTS_EVT_READ_PERMIT_REQ;
		} else if (p_ble_evt->evt.gatts_evt.params.authorize_request.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
			gatts_params.write.value_handle = rw_req.request.write.handle;
			gatts_params.write.data = rw_req.request.write.data;
			gatts_params.write.len = rw_req.request.write.len;
			gatts_params.write.offset = rw_req.request.write.offset;
			evt = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
		}

		mible_gatts_event_callback(evt, &gatts_params);
		
		errno = sd_ble_gatts_value_get(gatts_params.conn_handle, gatts_params.read.value_handle, &gatts_value);
		MI_ERR_CHECK(errno);

		if (BLE_GATTS_AUTHORIZE_TYPE_READ == p_ble_evt->evt.gatts_evt.params.authorize_request.type) {
			if (gatts_params.read.permit != true) {
				reply = (ble_gatts_rw_authorize_reply_params_t) {
					.type = BLE_GATTS_AUTHORIZE_TYPE_READ,
					.params.read.gatt_status = BLE_GATT_STATUS_ATTERR_READ_NOT_PERMITTED
				};
			} else {
				reply = (ble_gatts_rw_authorize_reply_params_t) {
					.type = BLE_GATTS_AUTHORIZE_TYPE_READ,
					.params.read.gatt_status = BLE_GATT_STATUS_SUCCESS,
					.params.read.p_data      = gatts_value.p_value,
					.params.read.len         = gatts_value.len,
					.params.read.offset      = gatts_value.offset
				};
			}
		} else if (BLE_GATTS_AUTHORIZE_TYPE_WRITE == p_ble_evt->evt.gatts_evt.params.authorize_request.type) {
			if (gatts_params.write.permit != true) {
				reply = (ble_gatts_rw_authorize_reply_params_t) {
					.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE,
					.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED
					};
			} else {
				reply = (ble_gatts_rw_authorize_reply_params_t) {
					.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE,
					.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS,
					.params.write.update      = 1,
					.params.write.p_data      = gatts_value.p_value,
					.params.write.len         = gatts_value.len,
					.params.write.offset      = gatts_value.offset
					};
			}
		}
		errno = sd_ble_gatts_rw_authorize_reply(gatts_params.conn_handle, &reply);
		MI_ERR_CHECK(errno);
		
		break;
		
	case BLE_GATTS_EVT_HVC:
		break;

	case BLE_GATTS_EVT_TIMEOUT:

		break;
	}

}

static void gattc_evt_dispatch(ble_evt_t *p_ble_evt)
{
	uint8_t gattc_evt_availble = 0;
	mible_gattc_evt_t evt;
	mible_gattc_evt_param_t gattc_params = {0};
	gattc_params.conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
    ble_gattc_evt_prim_srvc_disc_rsp_t prim_srv;
    ble_gattc_evt_hvx_t hvx;
    
	switch(p_ble_evt->header.evt_id) {
	case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
		evt = MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP;
		prim_srv = p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp;
		gattc_params.srv_disc_rsp.primary_srv_range.begin_handle = prim_srv.services[0].handle_range.start_handle;
		gattc_params.srv_disc_rsp.primary_srv_range.end_handle = prim_srv.services[0].handle_range.end_handle;
		uint8_t uuid_len, uuid128[16];
		sd_ble_uuid_encode(&prim_srv.services[0].uuid, &uuid_len, uuid128);
		gattc_params.srv_disc_rsp.srv_uuid.type = uuid_len == 16 ? 1 : 0;
		memcpy(gattc_params.srv_disc_rsp.srv_uuid.uuid128, uuid128, uuid_len);
		gattc_evt_availble = 1;
		break;

	case BLE_GATTC_EVT_HVX:
		hvx = p_ble_evt->evt.gattc_evt.params.hvx;
		if (hvx.type == BLE_GATT_HVX_INDICATION)
			evt = MIBLE_GATTC_EVT_INDICATION;
		else if (hvx.type == BLE_GATT_HVX_NOTIFICATION)
			evt = MIBLE_GATTC_EVT_NOTIFICATION;
		gattc_params.notification.handle = hvx.handle;
		gattc_params.notification.pdata  = hvx.data;
		gattc_params.notification.len    = hvx.len;
		gattc_evt_availble = 1;
		break;

	default:
		break;
	}

	if (gattc_evt_availble)
		mible_gattc_event_callback(evt, &gattc_params) ;

}

void mible_on_ble_evt(ble_evt_t *p_ble_evt)
{
    MI_LOG_DEBUG("BLE EVT %X\n", p_ble_evt->header.evt_id);
	gap_evt_dispatch(p_ble_evt);
	gatts_evt_dispatch(p_ble_evt);
	gattc_evt_dispatch(p_ble_evt);
}

void mible_gap_event_callback(mible_gap_evt_t evt,
    mible_gap_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gap_cb_table[user] != NULL) {
            m_gap_cb_table[user](evt, param);
        }
    }
}

void mible_gatts_event_callback(mible_gatts_evt_t evt,
    mible_gatts_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gatts_cb_table[user] != NULL) {
            m_gatts_cb_table[user](evt, param);
        }
    }
}

void mible_gattc_event_callback(mible_gattc_evt_t evt,
    mible_gattc_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gattc_cb_table[user] != NULL) {
            m_gattc_cb_table[user](evt, param);
        }
    }
}

void mible_arch_event_callback(mible_arch_event_t evt, 
		mible_arch_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_arch_cb_table[user] != NULL) {
            m_arch_cb_table[user](evt, param);
        }
    }
}

int mible_gap_register(mible_gap_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_gap_users == MIBLE_MAX_USERS)
    {
        ret = MI_ERR_RESOURCES;
    }
    else
    {
        m_gap_cb_table[m_gap_users] = cb;
        m_gap_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return MI_SUCCESS;
}

int mible_gattc_register(mible_gattc_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_gattc_users == MIBLE_MAX_USERS)
    {
        ret = MI_ERR_RESOURCES;
    }
    else
    {
        m_gattc_cb_table[m_gattc_users] = cb;
        m_gattc_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return MI_SUCCESS;
}

int mible_gatts_register(mible_gatts_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_gatts_users == MIBLE_MAX_USERS)
    {
        ret = MI_ERR_RESOURCES;
    }
    else
    {
        m_gatts_cb_table[m_gatts_users] = cb;
        m_gatts_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return MI_SUCCESS;
}

int mible_arch_register(mible_arch_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_arch_users == MIBLE_MAX_USERS)
    {
        ret = MI_ERR_RESOURCES;
    }
    else
    {
        m_arch_cb_table[m_arch_users] = cb;
        m_arch_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return MI_SUCCESS;
}

