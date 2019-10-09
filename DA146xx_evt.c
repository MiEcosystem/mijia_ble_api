#include <stdint.h>
#include <string.h>


#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_gatts.h"

#include "mi_config.h"
#include "mible_api.h"

#undef  MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME "Dialog"
#include "mible_log.h"

void gap_evt_dispatch(ble_evt_hdr_t *hdr)
{
	static uint16_t conn_handle;
	uint8_t gap_evt_availble = 0;
	mible_gap_evt_t evt;
	mible_gap_evt_param_t gap_params = {0};
	ble_evt_gap_conn_param_updated_t *connect_para_updated_evt = (ble_evt_gap_conn_param_updated_t *)hdr;
        ble_evt_gap_connected_t *dialog_evt = (ble_evt_gap_connected_t *)hdr;
        ble_evt_gap_disconnected_t *dialog_disc_evt = (ble_evt_gap_disconnected_t *)hdr;

	switch(hdr->evt_code) {
	case BLE_EVT_GAP_CONNECTED:
		evt = MIBLE_GAP_EVT_CONNECTED;
		conn_handle = dialog_evt->conn_idx;
		gap_params.conn_handle = conn_handle;
                gap_params.connect.type = dialog_evt->peer_address.addr_type == PUBLIC_ADDRESS ? MIBLE_ADDRESS_TYPE_PUBLIC : MIBLE_ADDRESS_TYPE_RANDOM;
		memcpy(gap_params.connect.peer_addr,  dialog_evt->peer_address.addr, 6);
		gap_params.connect.conn_param.max_conn_interval = dialog_evt->conn_params.interval_max;
		gap_params.connect.conn_param.min_conn_interval =  dialog_evt->conn_params.interval_min;
		gap_params.connect.conn_param.slave_latency     =  dialog_evt->conn_params.slave_latency;
		gap_params.connect.conn_param.conn_sup_timeout  =  dialog_evt->conn_params.sup_timeout;
                gap_params.connect.role =  MIBLE_GAP_PERIPHERAL;

		gap_evt_availble = 1;
		break;

	case BLE_EVT_GAP_DISCONNECTED:
		evt = MIBLE_GAP_EVT_DISCONNECT;

		conn_handle = dialog_disc_evt->conn_idx;
		gap_params.conn_handle = conn_handle;
		gap_params.disconnect.reason      = dialog_disc_evt->reason;
		gap_evt_availble = 1;
		break;

	case BLE_EVT_GAP_CONN_PARAM_UPDATED:
	        evt = MIBLE_GAP_EVT_CONN_PARAM_UPDATED;


		gap_params.conn_handle = conn_handle;
		gap_params.update_conn.conn_param.max_conn_interval = connect_para_updated_evt->conn_params.interval_max;
		gap_params.update_conn.conn_param.min_conn_interval = connect_para_updated_evt->conn_params.interval_min;
		gap_params.update_conn.conn_param.slave_latency     = connect_para_updated_evt->conn_params.slave_latency;
		gap_params.update_conn.conn_param.conn_sup_timeout  = connect_para_updated_evt->conn_params.sup_timeout;

		gap_evt_availble = 1;
		break;

	case BLE_EVT_GAP_ADV_COMPLETED:
		gap_evt_availble = 0;
		break;
	}
	
	if (gap_evt_availble)
		mible_gap_event_callback(evt, &gap_params) ;
	
}

 void gatts_evt_dispatch(ble_evt_hdr_t *hdr)
{
        bool gatts_evt_availble = false;
	mible_gatts_evt_t evt;
	mible_gatts_evt_param_t gatts_params = {0};
        ble_evt_gatts_write_req_t *dialog_evt = (ble_evt_gatts_write_req_t *)hdr;

	switch(hdr->evt_code) {
	case BLE_EVT_GATTS_WRITE_REQ:


	        gatts_params.conn_handle = dialog_evt->conn_idx;
		gatts_params.write.value_handle = dialog_evt->handle;
		gatts_params.write.data = dialog_evt->value;
		gatts_params.write.len = dialog_evt->length;
		gatts_params.write.offset = dialog_evt->offset;
		evt = MIBLE_GATTS_EVT_WRITE;

                gatts_evt_availble = true;
		break;
		
	default :
	        break;
	}

	if (gatts_evt_availble)
		mible_gatts_event_callback(evt, &gatts_params);
}



