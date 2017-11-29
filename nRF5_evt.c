#include <stdint.h>
#include <string.h>

#include "ble.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_gatts.h"

#include "mible_api.h"
#include "mible_log.h"

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
		
		gap_params.connect.type = p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr_type;
		memcpy(gap_params.connect.peer_addr, p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr, 6);;
		
		gap_params.connect.conn_param.max_conn_interval = p_ble_evt->evt.gap_evt.params.connected.conn_params.max_conn_interval;
		gap_params.connect.conn_param.min_conn_interval = p_ble_evt->evt.gap_evt.params.connected.conn_params.min_conn_interval;
		gap_params.connect.conn_param.slave_latency     = p_ble_evt->evt.gap_evt.params.connected.conn_params.slave_latency;
		gap_params.connect.conn_param.conn_sup_timeout  = p_ble_evt->evt.gap_evt.params.connected.conn_params.conn_sup_timeout;

		gap_params.connect.role = p_ble_evt->evt.gap_evt.params.connected.role;

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
	}
	
	if (gap_evt_availble)
		mible_gap_event_callback(evt, &gap_params) ;
	
}

static void gatts_evt_dispatch(ble_evt_t *p_ble_evt)
{
	uint32_t errno;
	uint8_t gatts_evt_availble = 0;
	mible_gatts_evt_t evt;
	mible_gatts_evt_param_t gatts_params = {0};
	gatts_params.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
	ble_gatts_evt_rw_authorize_request_t rw_req = {0};
	MI_LOG_INFO("[gatts_evt_dispatch]\r\n");
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

		if (gatts_params.write.permit == 0) 
			break;
		
		ble_gatts_value_t gatts_value = {0};
		errno = sd_ble_gatts_value_get(gatts_params.conn_handle, gatts_params.read.value_handle, &gatts_value);
		MI_ERR_CHECK(errno);

		ble_gatts_rw_authorize_reply_params_t reply = {0};
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
					.params.write.len         = gatts_value.len,
					.params.write.p_data      = gatts_value.p_value,
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

	switch(p_ble_evt->header.evt_id) {
	default:
		break;
	}

	if (gattc_evt_availble)
		mible_gattc_event_callback(evt, &gattc_params) ;

}

void mible_on_ble_evt(ble_evt_t *p_ble_evt)
{
	gap_evt_dispatch(p_ble_evt);
	gatts_evt_dispatch(p_ble_evt);
	gattc_evt_dispatch(p_ble_evt);
}
