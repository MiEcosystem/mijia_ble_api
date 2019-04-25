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
#include "mible_log.h"
#include "semaphore.h"
#include "arch_os.h"

#ifndef MIBLE_MAX_USERS
#define MIBLE_MAX_USERS 4
#endif


// Add for porting  
#define ADV_HANDLE   0 // ble adv handle 
#define CHAR_TABLE_NUM                  10
#define CHAR_DATA_LEN_MAX               20
uint8_t ble_scanning = 0;        // 1 means scaning; 0 means no scaning
static uint8_t ble_advertising = 0;
static uint8_t connection_handle = DISCONNECTION;
static uint8_t wait_discover_char = 0; // 1 
static uint8_t char_num = 0; 
static mible_uuid_t discover_srv_uuid = {0}; 
static arch_os_mutex_handle_t  stack_cmd_mutex; 

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

#include "mible_mesh_api.h"
void mible_stack_event_handler(struct gecko_cmd_packet *evt){
	mible_gap_evt_param_t gap_evt_param = {0};
	mible_gatts_evt_param_t gatts_evt_param = {0};
	//mible_gattc_evt_param_t gattc_evt_param = {0};
  
	if (NULL == evt) {
    	return;
  	}

  	/* Handle events */
  	switch (BGLIB_MSG_ID(evt->header)) {

		case gecko_evt_le_connection_opened_id:
			MI_LOG_INFO("gecko_evt_le_connection_opened_id \n"); 
        	connection_handle = evt->data.evt_le_connection_opened.connection;
        	gap_evt_param.conn_handle = evt->data.evt_le_connection_opened.connection;
        	memcpy(gap_evt_param.connect.peer_addr,
                evt->data.evt_le_connection_opened.address.addr, 6);
        	
                
			if((mible_gap_role_t) evt->data.evt_le_connection_opened.master == 0){
				gap_evt_param.connect.role = MIBLE_GAP_PERIPHERAL;
			}else{
				gap_evt_param.connect.role = MIBLE_GAP_CENTRAL;
			}
			
        	if ((evt->data.evt_le_connection_opened.address_type == le_gap_address_type_public)
                || (evt->data.evt_le_connection_opened.address_type
                        == le_gap_address_type_public_identity)) {
            	gap_evt_param.connect.type = MIBLE_ADDRESS_TYPE_PUBLIC;
        	} else {
            	gap_evt_param.connect.type = MIBLE_ADDRESS_TYPE_RANDOM;
        	}

        	mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &gap_evt_param);
			
			//struct gecko_msg_le_connection_set_parameters_rsp_t *param_ret;
			//param_ret = gecko_cmd_le_connection_set_parameters(connection_handle,
			//	0x28, 0x50, 4, 0x12c);
    	break;

    	case gecko_evt_le_connection_closed_id:
			
			MI_LOG_DEBUG("gecko_evt_le_connection_closed_id \n"); 
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
        	mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNECTED, &gap_evt_param);
    	break;

    	case gecko_evt_le_connection_parameters_id:
			MI_LOG_DEBUG("gecko_evt_le_connection_parameters_id \n"); 
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
			MI_LOG_DEBUG("connection parameters update: interval:0x%x, latency:%d, timeout: 0x%x\n",
					evt->data.evt_le_connection_parameters.interval,
					evt->data.evt_le_connection_parameters.latency,
					evt->data.evt_le_connection_parameters.timeout);

        	mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &gap_evt_param);
    	break;

    	case gecko_evt_le_gap_scan_response_id:{
        /* Invalid the connection handle since no connection yet*/
			if( evt->data.evt_le_gap_scan_response.data.len == 0 || 
					evt->data.evt_le_gap_scan_response.data.len >31)
				break; 
        	
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
		
			mible_mesh_event_callback(MIBLE_MESH_EVENT_ADV_PACKAGE, &gap_evt_param.report);
			
			uint8_t target_mac[6] = {0x19,0x2d,0xef,0x57,0x0b,0x00}; 	
			if(memcmp(target_mac, evt->data.evt_le_gap_scan_response.address.addr,6) == 0){
				//MI_LOG_INFO("Recieve target message. \n"); 
				//MI_HEXDUMP(evt->data.evt_le_gap_scan_response.data.data, 
						//evt->data.evt_le_gap_scan_response.data.len); 
			}	
		}
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
			
			MI_HEXDUMP(gatts_evt_param.write.data, gatts_evt_param.write.len); 
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

			MI_HEXDUMP(gatts_evt_param.write.data, gatts_evt_param.write.len); 
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
	cmd_mutex_get();
 	struct gecko_msg_system_get_bt_address_rsp_t *ret = gecko_cmd_system_get_bt_address();
	cmd_mutex_put();

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
	cmd_mutex_get();
	struct gecko_msg_system_set_bt_address_rsp_t *ret = gecko_cmd_system_set_bt_address(bt_addr);
	cmd_mutex_put();
	return ret->result;
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
	cmd_mutex_get();
	struct gecko_msg_le_connection_close_rsp_t *ret;
    if (connection_handle == DISCONNECTION) {
		cmd_mutex_put();
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_connection_close(conn_handle);
    if (ret->result == bg_err_success) {
		cmd_mutex_put();
        return MI_SUCCESS;
    } else if (ret->result == bg_err_invalid_conn_handle) {
		cmd_mutex_put();
        return MIBLE_ERR_INVALID_CONN_HANDLE;
    } else if (ret->result == bg_err_not_connected) {
		cmd_mutex_put();
        return MI_ERR_INVALID_STATE;
    } else {
		cmd_mutex_put();
        return MIBLE_ERR_UNKNOWN;
    }
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
	int ret = 0; 

	cmd_mutex_get();
    if (ble_scanning) {
		gecko_cmd_le_gap_end_procedure();
    }

    if (scan_type == MIBLE_SCAN_TYPE_PASSIVE) {
        active = 0;
    } else if (scan_type == MIBLE_SCAN_TYPE_ACTIVE) {
        active = 1;
    } else {
		cmd_mutex_put();
        return MI_ERR_INVALID_PARAM;
    }

    scan_interval = scan_param.scan_interval;
    scan_window = scan_param.scan_window;
	MI_LOG_WARNING("scan_interval = 0x%x \n", scan_param.scan_interval);
	MI_LOG_WARNING("scan_window = 0x%x \n", scan_param.scan_window);

	ret = gecko_cmd_le_gap_set_discovery_timing(1, scan_interval, scan_window)->result;
	if(ret != 0){
		MI_LOG_ERROR("set discovery timing error 0x%x \n", ret); 
		cmd_mutex_put();
		return ret;
	}

	ret = gecko_cmd_le_gap_set_discovery_type(1,active)->result;  
	if(ret != 0){
		MI_LOG_ERROR("set discovery type error 0x%x \n", ret);
		cmd_mutex_put();
		return ret;
	}
	uint8_t gap_type_list[] = {0x2A, 0x2B, 0x16, 0xFF}; 
	gecko_cmd_mesh_node_set_adv_event_filter(0x8000,sizeof(gap_type_list), gap_type_list); 
	ret = gecko_cmd_le_gap_start_discovery(1,le_gap_discover_observation)->result; 
	
	if( ret != 0){
		MI_LOG_ERROR("start_discovery error 0x%x \n", ret); 
		cmd_mutex_put();
		return ret;
	}

	cmd_mutex_put();
    ble_scanning = 1;
	MI_LOG_DEBUG("scan start return 0\n");
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
	cmd_mutex_get();
    gecko_cmd_le_gap_end_procedure();
	cmd_mutex_put();
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
	MI_LOG_DEBUG(" start advatising \n");  
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

	cmd_mutex_get();
    result = gecko_cmd_le_gap_set_advertise_timing(ADV_HANDLE, p_param->adv_interval_min,p_param->adv_interval_max, 0, 0)->result;
    /*MI_ERR_CHECK(result);*/
    if (result == bg_err_invalid_param) {
		cmd_mutex_put();
        return MI_ERR_INVALID_PARAM;
    }

    result = gecko_cmd_le_gap_set_advertise_channel_map(ADV_HANDLE, channel_map)->result;
    if (result == bg_err_invalid_param) {
		cmd_mutex_put();
        return MI_ERR_INVALID_PARAM;
    }

    if (p_param->adv_type == MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_connectable_scannable;
    } else if (p_param->adv_type == MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED) {
        connect = le_gap_scannable_non_connectable;
    } else if (p_param->adv_type == MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_non_connectable;
    } else {
		cmd_mutex_put();
        return MI_ERR_INVALID_PARAM;
    }

    if (last_adv_data.len != 0) {
        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0,
                last_adv_data.len, last_adv_data.data)->result;
        if (result != bg_err_success){
			cmd_mutex_put();
            return MI_ERR_BUSY;
		}
    }
    if (last_scan_rsp.len != 0) {
        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1,
                last_scan_rsp.len, last_scan_rsp.data)->result;
        if (result != bg_err_success){
			cmd_mutex_put();
            return MI_ERR_BUSY;
		}
    }

	gecko_cmd_le_gap_set_advertise_tx_power(ADV_HANDLE, 100);

    result = gecko_cmd_le_gap_start_advertising(ADV_HANDLE, le_gap_user_data, connect)->result;
    /*MI_ERR_CHECK(result);*/
    if (result == bg_err_success) {
        ble_advertising = 1;
		cmd_mutex_put();
        return MI_SUCCESS;
    } else {
		cmd_mutex_put();
        return MI_ERR_BUSY;
    }
	cmd_mutex_put();
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

	cmd_mutex_get();
    if (p_data != NULL && dlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set advertisement data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0, dlen, p_data);
        if (ret->result == bg_err_invalid_param) {
			cmd_mutex_put();
            return MI_ERR_INVALID_PARAM;
        }
        memcpy(last_adv_data.data, p_data, dlen);
        last_adv_data.len = dlen;
    }

    if (p_sr_data != NULL && srdlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set scan response data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1, srdlen, p_sr_data);
        if (ret->result == bg_err_invalid_param) {
			cmd_mutex_put();
            return MI_ERR_INVALID_PARAM;
        }
        memcpy(last_scan_rsp.data, p_sr_data, srdlen);
        last_scan_rsp.len = srdlen;
    }

	cmd_mutex_put();
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
	cmd_mutex_get();
	struct gecko_msg_le_gap_stop_advertising_rsp_t *ret;
    if (!ble_advertising) {
		cmd_mutex_put();
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_gap_stop_advertising(ADV_HANDLE);
    /*MI_ERR_CHECK(ret->result);*/
    if (ret->result == bg_err_success) {
		cmd_mutex_put();
        return MI_SUCCESS;
    } else {
		cmd_mutex_put();
        return MIBLE_ERR_UNKNOWN;
    }
	cmd_mutex_put();
    return MI_SUCCESS;
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

	cmd_mutex_get();
    ret = gecko_cmd_le_connection_set_parameters(conn_handle,
            conn_params.min_conn_interval, conn_params.max_conn_interval,
            conn_params.slave_latency, conn_params.conn_sup_timeout);
    if (ret->result == bg_err_success) {
		cmd_mutex_put();
        return MI_SUCCESS;
    } else if (ret->result == bg_err_invalid_conn_handle) {
		cmd_mutex_put();
        return MIBLE_ERR_INVALID_CONN_HANDLE;
    } else if (ret->result == bg_err_invalid_param) {
		cmd_mutex_put();
        return MI_ERR_INVALID_PARAM;
    } else if (ret->result == bg_err_wrong_state) {
		cmd_mutex_put();
        return MI_ERR_INVALID_STATE;
    }
	
	cmd_mutex_put();
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

	cmd_mutex_get();
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
		
		MI_LOG_DEBUG("user cmd: uuid = 0x%x\n", p_char_db->char_uuid.uuid16);
		rsp = gecko_cmd_user_message_to_target(5, data);  
        
		MI_LOG_DEBUG("user cmd: uuid = 0x%x\n", p_char_db->char_uuid.uuid16);

		memcpy(&p_char_db->char_value_handle,rsp->data.data,2);
		printf("uuid = 0x%x, handle = %d \n", 
				p_char_db->char_uuid.uuid16, p_char_db->char_value_handle);


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
            ret = MI_ERR_INTERNAL;
            goto exception;
        }
    }

    param.srv_init_cmp.p_gatts_db = p_server_db;
    param.srv_init_cmp.status = ret;
	MI_LOG_DEBUG("mible service init succ.\n"); 
    mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
	cmd_mutex_put();
	return 0;

exception:
	cmd_mutex_put();
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

	cmd_mutex_get();
    ret = gecko_cmd_gatt_server_send_characteristic_notification(conn_handle,
            char_value_handle, len, p_value);

	cmd_mutex_put();
    return ret->result;
}
#include <time.h>
#include <sys/time.h>
void get_time()
{
	struct timeval tv; 
	struct timezone tz; 
	gettimeofday(&tv, &tz);
	
	struct tm *tm_now;
    time_t now;
	time(&now);
	tm_now = localtime(&now);	
	char datetime[50] = {0};
 
    strftime(datetime, 50, "%H:%M:%S", tm_now);
	printf("[%s.%d]", datetime, (int)tv.tv_usec); 
}

void hexdump(uint8_t *base_addr, uint8_t bytes)
{
	get_time();
	if(base_addr == NULL || bytes == 0){
		return; 
	}
	for(uint8_t i=0;i<bytes;i++){
		printf("0x%2x ",base_addr[i]);
	}
	printf("\n"); 
  	return; 	
}
void cmd_mutex_init()
{
	arch_os_mutex_create(&stack_cmd_mutex);
}
void cmd_mutex_get()
{
	arch_os_mutex_get(stack_cmd_mutex, ARCH_OS_WAIT_FOREVER);
}

void cmd_mutex_put()
{
	arch_os_mutex_put(stack_cmd_mutex);
}
