#include <stdint.h>
#include <string.h>

#include "ble.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_gatts.h"

#include "mible_api.h"

#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "nRF"
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
        sd_ble_gatts_sys_attr_set(conn_handle, NULL, 0, 0);

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
		gap_params.disconnect.reason = (mible_gap_disconnect_reason_t)p_ble_evt->evt.gap_evt.params.disconnected.reason;
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
#if (NRF_SD_BLE_API_VERSION <= 3)
		gap_params.report.adv_type = adv.scan_rsp ? SCAN_RSP_DATA : ADV_DATA;
		memcpy(gap_params.report.data, adv.data, adv.dlen);;
		gap_params.report.data_len = adv.dlen;
#else
        gap_params.report.adv_type = adv.type.scan_response ? SCAN_RSP_DATA : ADV_DATA;
		memcpy(gap_params.report.data, adv.data.p_data, adv.data.len);;
		gap_params.report.data_len = adv.data.len;
#endif
		gap_params.report.rssi = adv.rssi;

		gap_evt_availble = 1;
		break;
	}
	
	if (gap_evt_availble)
		mible_gap_event_callback(evt, &gap_params) ;
	
}

static void gatts_evt_dispatch(ble_evt_t *p_ble_evt)
{
    bool gatts_evt_availble = false;
	mible_gatts_evt_t evt;
	mible_gatts_evt_param_t gatts_params = {0};
	gatts_params.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
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
        
        gatts_evt_availble = true;
		break;
		
	case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
		if (p_ble_evt->evt.gatts_evt.params.authorize_request.type == BLE_GATTS_AUTHORIZE_TYPE_READ) {
			gatts_params.read.value_handle = p_ble_evt->evt.gatts_evt.params.authorize_request.request.read.handle;
			evt = MIBLE_GATTS_EVT_READ_PERMIT_REQ;
		} else if (p_ble_evt->evt.gatts_evt.params.authorize_request.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
			gatts_params.write.value_handle = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.handle;
			gatts_params.write.data = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.data;
			gatts_params.write.len = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.len;
			gatts_params.write.offset = p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.offset;
			evt = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
		}

        gatts_evt_availble = true;
		break;
		
	case BLE_GATTS_EVT_HVC:
		break;

	case BLE_GATTS_EVT_TIMEOUT:

		break;
	}

	if (gatts_evt_availble)
		mible_gatts_event_callback(evt, &gatts_params) ;
}

static void gattc_evt_dispatch(ble_evt_t *p_ble_evt)
{
	uint8_t gattc_evt_availble = 0;
	mible_gattc_evt_t evt;
	mible_gattc_evt_param_t gattc_params = {0};
	gattc_params.conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
    uint8_t uuid_len, uuid128[16];

	switch(p_ble_evt->header.evt_id) {
	case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
		evt = MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP;
        ble_gattc_evt_prim_srvc_disc_rsp_t prim_srv;
		prim_srv = p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp;
		gattc_params.srv_disc_rsp.primary_srv_range.begin_handle = prim_srv.services[0].handle_range.start_handle;
		gattc_params.srv_disc_rsp.primary_srv_range.end_handle = prim_srv.services[0].handle_range.end_handle;

		sd_ble_uuid_encode(&prim_srv.services[0].uuid, &uuid_len, uuid128);
		gattc_params.srv_disc_rsp.srv_uuid.type = uuid_len == 16 ? 1 : 0;
		memcpy(gattc_params.srv_disc_rsp.srv_uuid.uuid128, uuid128, uuid_len);
		gattc_evt_availble = 1;
		break;

	case BLE_GATTC_EVT_HVX:
        evt = MIBLE_GATTC_EVT_INDICATION;
        ble_gattc_evt_hvx_t hvx = p_ble_evt->evt.gattc_evt.params.hvx;
		if (hvx.type == BLE_GATT_HVX_INDICATION)
			evt = MIBLE_GATTC_EVT_INDICATION;
		else if (hvx.type == BLE_GATT_HVX_NOTIFICATION)
			evt = MIBLE_GATTC_EVT_NOTIFICATION;
		gattc_params.notification.handle = hvx.handle;
		gattc_params.notification.pdata  = hvx.data;
		gattc_params.notification.len    = hvx.len;
		gattc_evt_availble = 1;
		break;

    case BLE_GATTC_EVT_CHAR_DISC_RSP:
        evt = MIBLE_GATTC_EVT_CHR_DISCOVER_RESP;
        MI_LOG_INFO("char disc cnt %d\n", p_ble_evt->evt.gattc_evt.params.char_disc_rsp.count);
        ble_gattc_char_t chars = p_ble_evt->evt.gattc_evt.params.char_disc_rsp.chars[0];
        gattc_params.char_disc_rsp.value_handle = chars.handle_value;
		
        sd_ble_uuid_encode(&chars.uuid, &uuid_len, uuid128);
		gattc_params.char_disc_rsp.char_uuid.type = uuid_len == 16 ? 1 : 0;
		memcpy(gattc_params.char_disc_rsp.char_uuid.uuid128, uuid128, uuid_len);

        if (chars.char_props.notify == 1 || 
            chars.char_props.indicate == 1) {
            gattc_params.char_disc_rsp.succ = 1;
        }

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
	gap_evt_dispatch(p_ble_evt);
	gatts_evt_dispatch(p_ble_evt);
	gattc_evt_dispatch(p_ble_evt);
}

