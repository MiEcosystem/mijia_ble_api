// Copyright [2017] [Beijing Xiaomi Mobile Software Co., Ltd]
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mible_api.h"
#include "mible_port.h"
#include "mible_type.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "erf32_api.h"

#ifndef MIBLE_MAX_USERS
#define MIBLE_MAX_USERS 4
#endif


// Add for porting  
#define ADV_HANDLE   0 // ble adv handle 
#define CHAR_TABLE_NUM                  10
#define CHAR_DATA_LEN_MAX               20
static uint8_t ble_scanning = 0;
static uint8_t ble_advertising = 0;
static uint8_t connection_handle = DISCONNECTION;
static uint8_t wait_discover_char = 0; // 1 
static uint8_t char_num = 0; 
static mible_uuid_t discover_srv_uuid = {0}; 
typedef struct {
	uint16_t handle;
	bool rd_author;		// read authorization. Enabel or Disable MIBLE_GATTS_READ_PERMIT_REQ event
	bool wr_author;     // write authorization. Enabel or Disable MIBLE_GATTS_WRITE_PERMIT_REQ event
	uint8_t char_property;
	uint8_t len;
	uint8_t data[CHAR_DATA_LEN_MAX];
} user_char_t;

struct{
	uint8_t num;
	user_char_t item[CHAR_TABLE_NUM];
}m_char_table;
// porting add end


/* GAP, GATTS, GATTC event callback function */
static uint8_t m_gap_users, m_gattc_users, m_gatts_users, m_arch_users;
static mible_gap_callback_t m_gap_cb_table[MIBLE_MAX_USERS] = {0};
static mible_gatts_callback_t m_gatts_cb_table[MIBLE_MAX_USERS] = {0};
static mible_gattc_callback_t m_gattc_cb_table[MIBLE_MAX_USERS] = {0};
static mible_arch_callback_t m_arch_cb_table[MIBLE_MAX_USERS] = {0};
int init_state = 0;

int mible_gap_register(mible_gap_callback_t cb)
{
    CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_gap_cb_table[i] == 0){
			m_gap_cb_table[i] = cb;
			m_gap_users++; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;  // full 

}

int mible_gap_unregister(mible_gap_callback_t cb)
{
	
	CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_gap_cb_table[i] == cb){
			m_gap_cb_table[i] = 0;
			m_gap_users--; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;
}

int mible_gattc_register(mible_gattc_callback_t cb)
{
    CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_gattc_cb_table[i] == 0){
			m_gattc_cb_table[i] = cb;
			m_gattc_users++; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;  // full 

}

int mible_gattc_unregister(mible_gattc_callback_t cb)
{
	
	CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_gattc_cb_table[i] == cb){
			m_gattc_cb_table[i] = 0;
			m_gattc_users--; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;
}

int mible_gatts_register(mible_gatts_callback_t cb)
{
    CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_gatts_cb_table[i] == 0){
			m_gatts_cb_table[i] = cb;
			m_gatts_users++; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;  // full 
	

}
int mible_gatts_unregister(mible_gatts_callback_t cb)
{
	CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_gatts_cb_table[i] == cb){
			m_gatts_cb_table[i] = 0;
			m_gatts_users--; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;
}

int mible_arch_register(mible_arch_callback_t cb)
{
    CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_arch_cb_table[i] == 0){
			m_arch_cb_table[i] = cb;
			m_arch_users++; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;  // full 
}


int mible_arch_unregister(mible_arch_callback_t cb)
{
	
	CRITICAL_SECTION_ENTER();
	int i=0;
	while(i < MIBLE_MAX_USERS){
		if(m_arch_cb_table[i] == cb){
			m_arch_cb_table[i] = 0;
			m_arch_users--; 
			CRITICAL_SECTION_EXIT();
			return MI_SUCCESS;
		}
		i++;
	}
	CRITICAL_SECTION_EXIT();
	return -1;
}
/**
 *@brief    This function is MIBLE GAP related event callback function.
 *@param    [in] evt : GAP EVENT
 *          [in] param : callback parameters corresponding to evt
 *@return   Void
 *@note     You should support this function in your own ble stack .
 *          Make sure when the corresponding event occurs, be able to call this
 *function
 *          and pass in the corresponding parameters.
 */

void mible_gap_event_callback(mible_gap_evt_t evt, mible_gap_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gap_cb_table[user] != 0) {
            m_gap_cb_table[user](evt, param);
        }
    }
}

/**
 *@brief    This function is MIBLE GATTS related event callback function.
 *@param    [in] evt : GATTS EVENT
 *          [in] param : callback parameters corresponding to evt
 *@return   Void
 *@note     You should support this function in your own ble stack .
 Make sure when the corresponding event occurs, be able to call this
 function and pass in the corresponding parameters.
 */
void mible_gatts_event_callback(mible_gatts_evt_t evt,
        mible_gatts_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gatts_cb_table[user] != 0) {
            m_gatts_cb_table[user](evt, param);
        }
    }
}

/**
 *@brief    This function is MIBLE GATTC related event callback function.
 *@param    [in] evt : GATTC EVENT
 *          [in] param : callback parameters corresponding to evt
 *@return   Void
 *@note     You should support this function in your own ble stack .
 Make sure when the corresponding event occurs, be able to call this
 function and pass in the corresponding parameters.
 */
void mible_gattc_event_callback(mible_gattc_evt_t evt,
        mible_gattc_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gattc_cb_table[user] != 0) {
            m_gattc_cb_table[user](evt, param);
        }
    }
}

/*
 *@brief    This function is mible_arch api related event callback function.
 *@param    [in] evt: asynchronous function complete event
 *          [in] param: the return of asynchronous function
 *@note     You should support this function in corresponding asynchronous function.
 *          For now, mible_gatts_service_int and mible_record_write is asynchronous.
 * */
void mible_arch_event_callback(mible_arch_event_t evt,
        mible_arch_evt_param_t* param)
{
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_arch_cb_table[user] != 0) {
            m_arch_cb_table[user](evt, param);
        }
    }
}



void mible_stack_event_handler(struct gecko_cmd_packet *evt){
	mible_gap_evt_param_t gap_evt_param = {0};
	mible_gatts_evt_param_t gatts_evt_param = {0};
	mible_gattc_evt_param_t gattc_evt_param = {0};
  
	if (NULL == evt) {
    	return;
  	}

  	/* Handle events */
  	switch (BGLIB_MSG_ID(evt->header)) {

		case gecko_evt_le_connection_opened_id:
        	connection_handle = evt->data.evt_le_connection_opened.connection;
        	gap_evt_param.conn_handle = evt->data.evt_le_connection_opened.connection;
        	memcpy(gap_evt_param.connect.peer_addr,
                evt->data.evt_le_connection_opened.address.addr, 6);
        	gap_evt_param.connect.role =
                (mible_gap_role_t) evt->data.evt_le_connection_opened.master;
        	if ((evt->data.evt_le_connection_opened.address_type == le_gap_address_type_public)
                || (evt->data.evt_le_connection_opened.address_type
                        == le_gap_address_type_public_identity)) {
            	gap_evt_param.connect.type = MIBLE_ADDRESS_TYPE_PUBLIC;
        	} else {
            	gap_evt_param.connect.type = MIBLE_ADDRESS_TYPE_RANDOM;
        	}

        	mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &gap_evt_param);
    	break;

    	case gecko_evt_le_connection_closed_id:
        	connection_handle = DISCONNECTION;
        	gap_evt_param.conn_handle = evt->data.evt_le_connection_closed.connection;
        	if (evt->data.evt_le_connection_closed.reason == bg_err_bt_connection_timeout) {
            	gap_evt_param.disconnect.reason = CONNECTION_TIMEOUT;
        	} else if (evt->data.evt_le_connection_closed.reason
                == bg_err_bt_remote_user_terminated) {
            	gap_evt_param.disconnect.reason = REMOTE_USER_TERMINATED;
        	} else if (evt->data.evt_le_connection_closed.reason
                == bg_err_bt_connection_terminated_by_local_host) {
            	gap_evt_param.disconnect.reason = LOCAL_HOST_TERMINATED;
        	}
        	mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNET, &gap_evt_param);
    	break;

    	case gecko_evt_le_connection_parameters_id:
        	gap_evt_param.conn_handle =
                evt->data.evt_le_connection_parameters.connection;
        	gap_evt_param.update_conn.conn_param.min_conn_interval =
                evt->data.evt_le_connection_parameters.interval;
        	gap_evt_param.update_conn.conn_param.max_conn_interval =
                evt->data.evt_le_connection_parameters.interval;
        	gap_evt_param.update_conn.conn_param.slave_latency =
                evt->data.evt_le_connection_parameters.latency;
        	gap_evt_param.update_conn.conn_param.conn_sup_timeout =
                evt->data.evt_le_connection_parameters.timeout;

        	mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &gap_evt_param);
    	break;

    	case gecko_evt_le_gap_scan_response_id:
        /* Invalid the connection handle since no connection yet*/
        	gap_evt_param.conn_handle = INVALID_CONNECTION_HANDLE;
        	memcpy(gap_evt_param.report.peer_addr,
                evt->data.evt_le_gap_scan_response.address.addr, 6);
        	gap_evt_param.report.addr_type =
                (mible_addr_type_t) evt->data.evt_le_gap_scan_response.address_type;
        	if (evt->data.evt_le_gap_scan_response.packet_type == 0x04) {
            	gap_evt_param.report.adv_type = SCAN_RSP_DATA;
        	} else {
            	gap_evt_param.report.adv_type = ADV_DATA;
        	}
        	gap_evt_param.report.rssi = evt->data.evt_le_gap_scan_response.rssi;
        	memcpy(gap_evt_param.report.data, evt->data.evt_le_gap_scan_response.data.data,
                evt->data.evt_le_gap_scan_response.data.len);
        	gap_evt_param.report.data_len = evt->data.evt_le_gap_scan_response.data.len;

        	mible_gap_event_callback(MIBLE_GAP_EVT_ADV_REPORT, &gap_evt_param);
    	break;

    	case gecko_evt_gatt_server_attribute_value_id: {

        	uint16_t char_handle = evt->data.evt_gatt_server_attribute_value.attribute;
        	mible_gatts_evt_t event;

        	//uint8_t index = SearchDatabaseFromHandle(char_handle);
        	gatts_evt_param.conn_handle =
                evt->data.evt_gatt_server_attribute_value.connection;
        	gatts_evt_param.write.data =
                evt->data.evt_gatt_server_attribute_value.value.data;
        	gatts_evt_param.write.len = evt->data.evt_gatt_server_attribute_value.value.len;
        	gatts_evt_param.write.offset = evt->data.evt_gatt_server_attribute_value.offset;
        	gatts_evt_param.write.value_handle = char_handle;

        	for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
        		if(m_char_table.item[i].handle == char_handle){
        			if(m_char_table.item[i].wr_author == true){
        				event = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
        			}else {
                    	event = MIBLE_GATTS_EVT_WRITE;
                	}
        			mible_gatts_event_callback(event, &gatts_evt_param);
        			break;
        		}
        	}
    	}
    	break;

    	case gecko_evt_gatt_server_user_read_request_id: {

        	uint16_t char_handle = 0;
        	char_handle = evt->data.evt_gatt_server_user_read_request.characteristic;
			//printf("char_handle = 0x%x \n", char_handle);
        	for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){

        		if(m_char_table.item[i].handle == char_handle){

        			if(m_char_table.item[i].rd_author == true){
                    	
						gatts_evt_param.conn_handle =
                            evt->data.evt_gatt_server_user_read_request.connection;

                    	gatts_evt_param.read.value_handle =
                            evt->data.evt_gatt_server_user_read_request.characteristic;

                    	mible_gatts_event_callback(MIBLE_GATTS_EVT_READ_PERMIT_REQ, &gatts_evt_param);
        			}else{
                    	/* Send read response here since no application reaction needed*/
                    	if (evt->data.evt_gatt_server_user_read_request.offset>m_char_table.item[i].len) {
							
                        	gecko_cmd_gatt_server_send_user_read_response(
                                evt->data.evt_gatt_server_user_read_request.connection,
                                evt->data.evt_gatt_server_user_read_request.characteristic,
                                (uint8_t)bg_err_att_invalid_offset, 0, NULL);
                    	} else {

                        	gecko_cmd_gatt_server_send_user_read_response(
                                evt->data.evt_gatt_server_user_read_request.connection,
                                evt->data.evt_gatt_server_user_read_request.characteristic,
                                bg_err_success,
								m_char_table.item[i].len
										- evt->data.evt_gatt_server_user_read_request.offset,
								m_char_table.item[i].data
                                        + evt->data.evt_gatt_server_user_read_request.offset);
                    	}
            		}
        			break;
        		}
        	}
    	}
    	break;

    	case gecko_evt_gatt_server_user_write_request_id: {

        	uint16_t char_handle = evt->data.evt_gatt_server_attribute_value.attribute;
        	mible_gatts_evt_t event;

        	//uint8_t index = SearchDatabaseFromHandle(char_handle);
        	gatts_evt_param.conn_handle =
                evt->data.evt_gatt_server_attribute_value.connection;
        	gatts_evt_param.write.data =
                evt->data.evt_gatt_server_attribute_value.value.data;
        	gatts_evt_param.write.len = evt->data.evt_gatt_server_attribute_value.value.len;
        	gatts_evt_param.write.offset = evt->data.evt_gatt_server_attribute_value.offset;
        	gatts_evt_param.write.value_handle = char_handle;

        	for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
        		if(m_char_table.item[i].handle == char_handle){
        			if(m_char_table.item[i].wr_author == true){
        				event = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
        			}else {
                    	event = MIBLE_GATTS_EVT_WRITE;
                    	if((m_char_table.item[i].char_property & MIBLE_WRITE) != 0){

                    		memcpy(m_char_table.item[i].data + gatts_evt_param.write.offset,
                    			gatts_evt_param.write.data, gatts_evt_param.write.len);

                    		gecko_cmd_gatt_server_send_user_write_response(
                    			evt->data.evt_gatt_server_user_read_request.connection,
								evt->data.evt_gatt_server_user_read_request.characteristic,
                                bg_err_success);
                    	}
                	}
        			mible_gatts_event_callback(event, &gatts_evt_param);
        			break;
        		}
        	}
    	}
    	break;

    	case gecko_evt_gatt_server_characteristic_status_id:
			
        	if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == gatt_server_confirmation) {
            	/* Second parameter doesn't have any meaning */
            	gatts_evt_param.conn_handle = evt->data.evt_gatt_server_characteristic_status.connection;
            	gatts_evt_param.write.value_handle = evt->data.evt_gatt_server_characteristic_status.characteristic;
            	mible_gatts_event_callback(MIBLE_GATTS_EVT_IND_CONFIRM, &gatts_evt_param);
        	}
    	break;	
		case gecko_evt_gatt_characteristic_id:  
		break;
		case gecko_evt_gatt_service_id:
		break;
		case gecko_evt_gatt_procedure_completed_id:
		break;
		case gecko_evt_gatt_characteristic_value_id:
		break; 
    	default:
      		break;
  	}
}


/**
 *        GAP APIs
 */

/**
 * @brief   Get BLE mac address.
 * @param   [out] mac: pointer to data
 * @return  MI_SUCCESS          The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note:   You should copy gap mac to mac[6]
 * */
mible_status_t mible_gap_address_get(mible_addr_t mac)
{
 	struct gecko_msg_system_get_bt_address_rsp_t *ret = gecko_cmd_system_get_bt_address();
    memcpy(mac, ret->address.addr, 6);
    return MI_SUCCESS;
}

/**
 * @brief   Set BLE mac address.
 * @param   [in] mac: pointer to data
 * @return  MI_SUCCESS          The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note:   You should copy mac to gap mac[6]
 * */
mible_status_t mible_gap_address_set(mible_addr_t mac)
{
	bd_addr bt_addr; 
	memcpy(bt_addr.addr,(uint8_t *)mac,6);  
	struct gecko_msg_system_set_bt_address_rsp_t *ret = gecko_cmd_system_set_bt_address(bt_addr);
	return ret->result;
}

/**
 * @brief   Start scanning
 * @param   [in] scan_type: passive or active scaning
 *          [in] scan_param: scan parameters including interval, windows
 * and timeout
 * @return  MI_SUCCESS             Successfully initiated scanning procedure.
 *          MI_ERR_INVALID_STATE   Has initiated scanning procedure.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending
 * events and retry.
 * @note    Other default scanning parameters : public address, no
 * whitelist.
 *          The scan response is given through
 * MIBLE_GAP_EVT_ADV_REPORT event
 */
mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
        mible_gap_scan_param_t scan_param)
{
	uint16_t scan_interval, scan_window;
    uint8_t active;

    if (ble_scanning) {
        return MI_ERR_INVALID_STATE;
    }

    if (scan_type == MIBLE_SCAN_TYPE_PASSIVE) {
        active = 0;
    } else if (scan_type == MIBLE_SCAN_TYPE_ACTIVE) {
        active = 1;
    } else {
        return MI_ERR_INVALID_PARAM;
    }

    scan_interval = scan_param.scan_interval;
    scan_window = scan_param.scan_window;

	if(gecko_cmd_le_gap_set_discovery_timing(1, scan_interval, scan_window)->result != 0){
		return MI_ERR_INVALID_PARAM;
	}
	
	if(gecko_cmd_le_gap_set_discovery_type(1,1)->result != 0){
		return MI_ERR_INVALID_PARAM;
	}

	if(gecko_cmd_le_gap_start_discovery(1,le_gap_discover_observation)->result){
		return MIBLE_ERR_UNKNOWN;
	}

    ble_scanning = 1;
    return MI_SUCCESS;
}

/**
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void)
{
	if (!ble_scanning) {
        return MI_ERR_INVALID_STATE;
    }
    gecko_cmd_le_gap_end_procedure();
    ble_scanning = 0;
    return MI_SUCCESS;
}


typedef struct {
    uint8_t len;
    uint8_t data[31];
} mible_adv_data_t;
static mible_adv_data_t last_adv_data;
static mible_adv_data_t last_scan_rsp;
/**
 * @brief   Start advertising
 * @param   [in] p_adv_param : pointer to advertising parameters, see
 * mible_gap_adv_param_t for details
 * @return  MI_SUCCESS             Successfully initiated advertising procedure.
 *          MI_ERR_INVALID_STATE   Initiated connectable advertising procedure
 * when connected.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 * retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again.
 * @note    Other default advertising parameters: local public address , no
 * filter policy
 * */
mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
	uint16_t result;
    uint8_t channel_map = 0, connect = 0;

    if (p_param->ch_mask.ch_37_off != 1) {
        channel_map |= 0x01;
    }
    if (p_param->ch_mask.ch_38_off != 1) {
        channel_map |= 0x02;
    }
    if (p_param->ch_mask.ch_39_off != 1) {
        channel_map |= 0x04;
    }

    if ((connection_handle != DISCONNECTION)
            && (p_param->adv_type == MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED)) {
        return MI_ERR_INVALID_STATE;
    }

    result = gecko_cmd_le_gap_set_advertise_timing(ADV_HANDLE, p_param->adv_interval_min,p_param->adv_interval_max, 0, 0)->result;
    /*MI_ERR_CHECK(result);*/
    if (result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    }

    result = gecko_cmd_le_gap_set_advertise_channel_map(ADV_HANDLE, channel_map)->result;
    if (result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    }

    if (p_param->adv_type == MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_connectable_scannable;
    } else if (p_param->adv_type == MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED) {
        connect = le_gap_scannable_non_connectable;
    } else if (p_param->adv_type == MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_non_connectable;
    } else {
        return MI_ERR_INVALID_PARAM;
    }

    if (last_adv_data.len != 0) {
        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0,
                last_adv_data.len, last_adv_data.data)->result;
        if (result != bg_err_success)
            return MI_ERR_BUSY;
    }
    if (last_scan_rsp.len != 0) {
        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1,
                last_scan_rsp.len, last_scan_rsp.data)->result;
        if (result != bg_err_success)
            return MI_ERR_BUSY;
    }

	gecko_cmd_le_gap_set_advertise_tx_power(ADV_HANDLE, 100);

    result = gecko_cmd_le_gap_start_advertising(ADV_HANDLE, le_gap_user_data, connect)->result;
    /*MI_ERR_CHECK(result);*/
    if (result == bg_err_success) {
        ble_advertising = 1;
        return MI_SUCCESS;
    } else {
        return MI_ERR_BUSY;
    }
    return MI_SUCCESS;
}

/**
 * @brief   Config advertising data
 * @param   [in] p_data : Raw data to be placed in advertising packet. If NULL, no changes are made to the current advertising packet.
 * @param   [in] dlen   : Data length for p_data. Max size: 31 octets. Should be 0 if p_data is NULL, can be 0 if p_data is not NULL.
 * @param   [in] p_sr_data : Raw data to be placed in scan response packet. If NULL, no changes are made to the current scan response packet data.
 * @param   [in] srdlen : Data length for p_sr_data. Max size: BLE_GAP_ADV_MAX_SIZE octets. Should be 0 if p_sr_data is NULL, can be 0 if p_data is not NULL.
 * @return  MI_SUCCESS             Successfully set advertising data.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * */
mible_status_t mible_gap_adv_data_set(uint8_t const * p_data,
        uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
	struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t *ret;

    if (p_data != NULL && dlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set advertisement data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0, dlen, p_data);
        if (ret->result == bg_err_invalid_param) {
            return MI_ERR_INVALID_PARAM;
        }
        memcpy(last_adv_data.data, p_data, dlen);
        last_adv_data.len = dlen;
    }

    if (p_sr_data != NULL && srdlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set scan response data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1, srdlen, p_sr_data);
        if (ret->result == bg_err_invalid_param) {
            return MI_ERR_INVALID_PARAM;
        }
        memcpy(last_scan_rsp.data, p_sr_data, srdlen);
        last_scan_rsp.len = srdlen;
    }

    return MI_SUCCESS;
}

/**
 * @brief   Stop advertising
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void)
{
	struct gecko_msg_le_gap_stop_advertising_rsp_t *ret;
    if (!ble_advertising) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_gap_stop_advertising(ADV_HANDLE);
    /*MI_ERR_CHECK(ret->result);*/
    if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else {
        return MIBLE_ERR_UNKNOWN;
    }
    return MI_SUCCESS;
}

/**
 * @brief   Create a Direct connection
 * @param   [in] scan_param : scanning parameters, see TYPE
 * mible_gap_scan_param_t for details.
 *          [in] conn_param : connection parameters, see TYPE
 * mible_gap_connect_t for details.
 * @return  MI_SUCCESS             Successfully initiated connection procedure.
 *          MI_ERR_INVALID_STATE   Initiated connection procedure in connected state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again
 *          MIBLE_ERR_GAP_INVALID_BLE_ADDR    Invalid Bluetooth address
 * supplied.
 * @note    Own and peer address are both public.
 *          The connection result is given by MIBLE_GAP_EVT_CONNECTED
 * event
 * */
mible_status_t mible_gap_connect(mible_device_param_t *device, mible_gap_conn_param_t *p_conn_param)
{
	if(device == NULL || p_conn_param == NULL)
		return MI_ERR_INVALID_PARAM;
	// set conn params 
 	struct gecko_msg_le_gap_set_conn_parameters_rsp_t *set_conn_rsp = 
		gecko_cmd_le_gap_set_conn_parameters(p_conn_param->min_conn_interval,
			p_conn_param->max_conn_interval, p_conn_param->slave_latency, 
			p_conn_param->conn_sup_timeout);
 	if(set_conn_rsp->result != 0){
		return set_conn_rsp->result; 
	}	
	bd_addr bt_addr;
	memcpy(bt_addr.addr,device->addr,6); 
	struct gecko_msg_le_gap_connect_rsp_t *conn_rsp = 
		gecko_cmd_le_gap_connect(bt_addr, device->type, 1);
	return conn_rsp->result;
}

/**
 * @brief   Disconnect from peer
 * @param   [in] conn_handle: the connection handle
 * @return  MI_SUCCESS             Successfully disconnected.
 *          MI_ERR_INVALID_STATE   Not in connnection.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note    This function can be used by both central role and periphral
 * role.
 * */
mible_status_t mible_gap_disconnect(uint16_t conn_handle)
{
	struct gecko_msg_le_connection_close_rsp_t *ret;
    if (connection_handle == DISCONNECTION) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_connection_close(conn_handle);
    if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else if (ret->result == bg_err_invalid_conn_handle) {
        return MIBLE_ERR_INVALID_CONN_HANDLE;
    } else if (ret->result == bg_err_not_connected) {
        return MI_ERR_INVALID_STATE;
    } else {
        return MIBLE_ERR_UNKNOWN;
    }
}

/**
 * @brief   Update the connection parameters.
 * @param   [in] conn_handle: the connection handle.
 *          [in] conn_params: the connection parameters.
 * @return  MI_SUCCESS             The Connection Update procedure has been
 *started successfully.
 *          MI_ERR_INVALID_STATE   Initiated this procedure in disconnected
 *state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 *retry.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note    This function can be used by both central role and peripheral
 *role.
 * */
mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
        mible_gap_conn_param_t conn_params)
{
	struct gecko_msg_le_connection_set_parameters_rsp_t *ret;
    if (connection_handle == DISCONNECTION) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_connection_set_parameters(conn_handle,
            conn_params.min_conn_interval, conn_params.max_conn_interval,
            conn_params.slave_latency, conn_params.conn_sup_timeout);
    if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else if (ret->result == bg_err_invalid_conn_handle) {
        return MIBLE_ERR_INVALID_CONN_HANDLE;
    } else if (ret->result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    } else if (ret->result == bg_err_wrong_state) {
        return MI_ERR_INVALID_STATE;
    }
    return MI_SUCCESS;
}

/**
 *        GATT Server APIs
 */

/**
 * @brief   Add a Service to a GATT server
 * @param   [in|out] p_server_db: pointer to mible service data type
 * of mible_gatts_db_t, see TYPE mible_gatts_db_t for details.
 * @return  MI_SUCCESS             Successfully added a service declaration.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_NO_MEM          Not enough memory to complete operation.
 * @note    This function can be implemented asynchronous. When service inition complete, call mible_arch_event_callback function and pass in MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP event and result.
 * */
mible_status_t mible_gatts_service_init(mible_gatts_db_t *p_server_db)
{
	mible_status_t ret = MI_SUCCESS;
    mible_arch_evt_param_t param;

    mible_gatts_char_db_t * p_char_db = p_server_db->p_srv_db->p_char_db;

    for(int i = 0;
            i < p_server_db->p_srv_db->char_num && i < CHAR_TABLE_NUM;
            i++, p_char_db++) 
	{	
		// 0B: ID ; 
		// 1B-2B: service uuid 
		// 3B-4B: characteristic uuid
		uint8_t data[5] = {0};
		data[0] = USER_CMD_SERVICE_INIT;
		memcpy(data+1,&(p_server_db->p_srv_db->srv_uuid.uuid16),2);
		memcpy(data+3,&(p_char_db->char_uuid.uuid16),2);
		struct gecko_msg_user_message_to_target_rsp_t *rsp;
		rsp = gecko_cmd_user_message_to_target(5, data);  
        
		memcpy(&p_char_db->char_value_handle,rsp->data.data,2);
		//printf("uuid = 0x%x, handle = 0x%x \n", 
		//		p_char_db->char_uuid.uuid16, p_char_db->char_value_handle);

        if (rsp->result == bg_err_success) {
            memcpy(&p_char_db->char_value_handle,rsp->data.data,2);
			
            m_char_table.item[i].handle = p_char_db->char_value_handle;
            m_char_table.item[i].rd_author = p_char_db->rd_author;
            m_char_table.item[i].wr_author = p_char_db->wr_author;
            m_char_table.item[i].char_property = p_char_db->char_property;
            m_char_table.item[i].len = p_char_db->char_value_len;
            memcpy(m_char_table.item[i].data, p_char_db->p_value, p_char_db->char_value_len);
            m_char_table.num = 1 + i;
        } else {
            /*MI_LOG_ERROR("no char %d found.\n", p_char_db->char_uuid.uuid16);*/
            ret = MI_ERR_INTERNAL;
            goto exception;
        }
    }

    param.srv_init_cmp.p_gatts_db = p_server_db;
    param.srv_init_cmp.status = ret;
    mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);

exception:
    return ret;
}

/**
 * @brief   Set characteristic value
 * @param   [in] srv_handle: service handle
 *          [in] value_handle: characteristic value handle
 *          [in] offset: the offset from which the attribute value has
 *to be updated
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully retrieved the value of the
 *attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE  Attributes are not modifiable by
 *the application.
 * */
mible_status_t mible_gatts_value_set(uint16_t srv_handle,
        uint16_t value_handle, uint8_t offset, uint8_t* p_value, uint8_t len)
{
	if (p_value == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

/*    if(!is_vaild_handle(value_handle)){*/
        /*return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;*/
    /*}*/


    for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
    	if(m_char_table.item[i].handle == value_handle){
    		if(m_char_table.item[i].len >= offset + len){
    			memcpy(m_char_table.item[i].data + offset, p_value, len);
    			return MI_SUCCESS;
    		}else{
    			return MI_ERR_INVALID_LENGTH;
    		}
    	}
    }
    return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
}

/**
 * @brief   Get charicteristic value as a GATTS.
 * @param   [in] srv_handle: service handle
 *          [in] value_handle: characteristic value handle
 *          [out] p_value: pointer to data which stores characteristic value
 *          [out] p_len: pointer to data length.
 * @return  MI_SUCCESS             Successfully get the value of the attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 **/
mible_status_t mible_gatts_value_get(uint16_t srv_handle,
        uint16_t value_handle, uint8_t* p_value, uint8_t *p_len)
{
	if (p_value == NULL || p_len == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

/*    if(!is_vaild_handle(value_handle)){*/
        /*return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;*/
    /*}*/

    for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
    	if(m_char_table.item[i].handle == value_handle){
    		*p_len = m_char_table.item[i].len;
    		memcpy(p_value, m_char_table.item[i].data, *p_len);
    		return MI_SUCCESS;
    	}
    }
    return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
}

/**
 * @brief   Set characteristic value and notify it to client.
 * @param   [in] conn_handle: conn handle
 *          [in] srv_handle: service handle
 *          [in] char_value_handle: characteristic  value handle
 *          [in] offset: the offset from which the attribute value has to
 * be updated
 *          [in] p_value: pointer to data
 *          [in] len: data length
 *          [in] type : notification = 1; indication = 2;
 *
 * @return  MI_SUCCESS             Successfully queued a notification or
 * indication for transmission,
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State or notifications
 * and/or indications not enabled in the CCCD.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE   //Attributes are not modifiable by
 * the application.
 * @note    This function checks for the relevant Client Characteristic
 * Configuration descriptor value to verify that the relevant operation (notification or
 * indication) has been enabled by the client.
 * */
mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle,
        uint16_t srv_handle, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type)
{
	struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t *ret;

    if (p_value == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    if ((uint8_t) conn_handle == DISCONNECTION) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_gatt_server_send_characteristic_notification(conn_handle,
            char_value_handle, len, p_value);

    if (ret->result == bg_err_wrong_state) {
        return MI_ERR_INVALID_STATE;
    } else if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else {
        return MIBLE_ERR_UNKNOWN;
    }
    return MI_SUCCESS;
    return MI_SUCCESS;
}

/**
 * @brief   Respond to a Read/Write user authorization request.
 * @param   [in] conn_handle: conn handle
 *          [in] status:  1: permit to change value ; 0: reject to change value
 *          [in] char_value_handle: characteristic handle
 *          [in] offset: the offset from which the attribute value has to
 * be updated
 *          [in] p_value: Pointer to new value used to update the attribute value.
 *          [in] len: data length
 *          [in] type : read response = 1; write response = 2;
 *
 * @return  MI_SUCCESS             Successfully queued a response to the peer, and in the case of a write operation, GATT updated.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State or no authorization request pending.
 *          MI_ERR_INVALID_LENGTH  Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 * @note    This call should only be used as a response to a MIBLE_GATTS_EVT_READ/WRITE_PERMIT_REQ
 * event issued to the application.
 * */
__WEAK mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle,
        uint8_t status, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type)
{
    return MI_SUCCESS;
}

/**
 *        GATT Client APIs
 */

/**
 * @brief   Discover service by service UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] srv_uuid: service uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Primary
 *Service Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    The response is given through MIBLE_GATTC_EVT_DISCOVERY_RSP event
 * */
mible_status_t mible_gattc_service_discovery(uint16_t conn_handle, mible_uuid_t srv_uuid)
{
	struct gecko_msg_gatt_discover_primary_services_by_uuid_rsp_t *ret; 
	memcpy((uint8_t *)&discover_srv_uuid, (uint8_t *)&srv_uuid, sizeof(mible_uuid_t));
	if(srv_uuid.type == 0)
		ret = gecko_cmd_gatt_discover_primary_services_by_uuid(
				conn_handle, 2, (const uint8_t *)&srv_uuid.uuid16); 	
	else
		ret = gecko_cmd_gatt_discover_primary_services_by_uuid(
				conn_handle, 16 , (const uint8_t *)srv_uuid.uuid128);
	return ret->result;
}
/**
 * @brief   set peer device notification and indication enable or disable.
 * @param   [in] conn_handle: connect handle
 *          [in] char_handle: characteristic handle
 *			[in] desc_handle: cccd handle
 *			[in] value: cccd value
 * @return  MI_SUCCESS             Successfully started or resumed the Primary
 *Service Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    write peer with response, the response is given through MIBLE_GATTC_EVT_WRITE_RSP event
 * */
mible_status_t mible_gattc_set_cccd(uint16_t conn_handle, uint16_t char_handle,
       uint16_t desc_handle, uint16_t value)
{ 
	struct gecko_msg_gatt_set_characteristic_notification_rsp_t *ret; 
	ret = gecko_cmd_gatt_set_characteristic_notification(
			conn_handle, char_handle, value);
	return ret->result;
}



/**
 * @brief   Read characteristic value by handle
 * @param   [in] conn_handle: connnection handle
 *          [in] char_handle: characteristic handle 
 * @return  MI_SUCCESS             Successfully started or resumed the Read
 * using Characteristic UUID procedure.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through MIBLE_GATTC_EVT_READ_RSP event
 * */
mible_status_t mible_gattc_read_value(uint16_t conn_handle, uint16_t char_handle)
{
	struct gecko_msg_gatt_read_characteristic_value_rsp_t *ret;
	ret = gecko_cmd_gatt_read_characteristic_value(conn_handle, char_handle);
    return ret->result;
}

/**
 * @brief   Write value by handle with response
 * @param   [in] conn_handle: connection handle
 *          [in] char_handle: handle to the attribute to be written.
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write with response
 * procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through MIBLE_GATTC_EVT_WRITE_RESP event
 *
 * */
mible_status_t mible_gattc_write_value(uint16_t conn_handle, uint16_t char_handle,
        uint8_t* p_value, uint8_t len)
{
	struct gecko_msg_gatt_write_characteristic_value_rsp_t *ret; 
	ret = gecko_cmd_gatt_write_characteristic_value(conn_handle, char_handle, 
			len, p_value);
    return ret->result;
}

/**
 * @brief   Write value by handle without response
 * @param   [in] conn_handle: connection handle
 *          [in] char_handle: handle to the attribute to be written.
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write Cmd procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    no response
 * */
mible_status_t mible_gattc_write_without_response(uint16_t conn_handle, uint16_t char_handle,
        uint8_t* p_value, uint8_t len)
{
	struct gecko_msg_gatt_write_characteristic_value_without_response_rsp_t *ret;
	ret = gecko_cmd_gatt_write_characteristic_value_without_response(
			conn_handle, char_handle, len, p_value);
    return ret->result;
}

mible_status_t mible_gattc_confirm_indication(uint16_t conn_handle, uint16_t char_handle)
{
	struct gecko_msg_gatt_send_characteristic_confirmation_rsp_t *ret;
	ret = gecko_cmd_gatt_send_characteristic_confirmation(conn_handle);
	return ret->result;
}
