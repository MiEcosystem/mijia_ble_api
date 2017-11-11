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
#include "mible_type.h"

#include "nrf_soc.h"
#include "nrf_queue.h"

#include "ble_types.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_gatts.h"

#include "app_timer.h"

#include "mi_psm.h"

#define TIMER_MAX_NUM             4

typedef struct {
	mible_handler_t handler;
	void *arg;
} mible_task_t;

typedef struct {
	uint8_t is_avail;
	app_timer_t data;
} timer_item_t;

static timer_item_t m_timer_pool[TIMER_MAX_NUM] = {
	[0] = { .is_avail = 1},
	[1] = { .is_avail = 1},
	[2] = { .is_avail = 1},
	[3] = { .is_avail = 1}
};

static app_timer_t* find_timer()
{
	uint8_t i;
	for (i = 0; i < TIMER_MAX_NUM; i++) {
		if (m_timer_pool[i].is_avail) {
			m_timer_pool[i].is_avail = 0;
			return (void*)&m_timer_pool[i].data ;
		}
	}
	return NULL;
}

static int free_timer(void* timer_id)
{
	for (uint8_t i = 0; i < TIMER_MAX_NUM; i++) {
		if (timer_id == &m_timer_pool[i].data) {
			m_timer_pool[i].is_avail = 1;
			return i;
		}
	}
	return -1;
}

NRF_QUEUE_DEF(mible_task_t, task_queue, 4, NRF_QUEUE_MODE_OVERFLOW);

/*
 * Add your own include file
 *
 * */

/* GAP, GATTS, GATTC event callback function */

/**
 *@brief    This function is MIBLE GAP related event callback function.
 *@param    [in] evt : GAP EVENT
 *			[in] param : callback parameters corresponding to evt
 *@return   Void
 *@note     You should support this function in your own ble stack .
 *          Make sure when the corresponding event occurs, be able to call this
 *function
 *          and pass in the corresponding parameters.
*/
void mible_gap_event_callback(mible_gap_evt_t evt,
    mible_gap_evt_param_t* param)
{
    switch (evt) {
    case MIBLE_GAP_EVT_CONNECTED:
		mible_std_server_gap_evt_connected(param); 
        break;
    case MIBLE_GAP_EVT_DISCONNET:
		mible_std_server_gap_evt_disconnected(param);
        break;
    case MIBLE_GAP_EVT_ADV_REPORT:
        mible_std_server_gap_evt_scan_report(param); 
		break;
    case MIBLE_GAP_EVT_CONN_PARAM_UPDATED:
//		mible_std_server_gap_evt_conn_params_updated(param);
        break;
    }
}
/**
 *@brief    This function is MIBLE GATTS related event callback function.
 *@param    [in] evt : GATTS EVENT
 *			[in] param : callback parameters corresponding to evt
 *@return   Void
 *@note     You should support this function in your own ble stack .
            Make sure when the corresponding event occurs, be able to call this
 function and pass in the corresponding parameters.
*/
void mible_gatts_event_callback(mible_gatts_evt_t evt,
    mible_gatts_evt_param_t* param)
{
    switch (evt) {
    case MIBLE_GATTS_EVT_WRITE:
		// mible_std_server_gatts_evt_write(param->write);
        break;

    case MIBLE_GATTS_EVT_READ_PERMIT_REQ:
		// mible_std_server_gatts_evt_read_permit_req(param->read);
        break;

    case MIBLE_GATTS_EVT_WRITE_PERMIT_REQ:
		// mible_std_server_gatts_evt_write_permit_req(param->write);
        break;

	case MIBLE_GATTS_EVT_IND_CONFIRM:
		break;
    }
}

/**
 *@brief    This function is MIBLE GATTC related event callback function.
 *@param    [in] evt : GATTC EVENT
 *			[in] param : callback parameters corresponding to evt
 *@return   Void
 *@note     You should support this function in your own ble stack .
            Make sure when the corresponding event occurs, be able to call this
 function and pass in the corresponding parameters.
*/
void mible_gattc_event_callback(mible_gattc_evt_t evt,
    mible_gattc_evt_param_t* param)
{
    switch (evt) {
    case MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP:
        break;
    case MIBLE_GATTC_EVT_CHR_DISCOVER_BY_UUID_RESP:
        break;
    case MIBLE_GATTC_EVT_CCCD_DISCOVER_RESP:
        break;
    case MIBLE_GATTC_EVT_READ_CHAR_VALUE_BY_UUID_RESP:
        break;
    case MIBLE_GATTC_EVT_WRITE_RESP:
        break;
    default:
		;
    }
}
/*
 *@brief 	This function is mible_arch api related event callback function.
 *@param 	[in] evt: asynchronous function complete event 
 *			[in] param: the return of asynchronous function 
 *@note  	You should support this function in corresponding asynchronous function. 
 *          For now, mible_gatts_service_int and mible_record_write is asynchronous. 
 * */
void mible_arch_event_callback(mible_arch_event_t evt, 
		mible_arch_evt_param_t* param)
{
	switch(evt) {
		case MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP:
		break;
		case MIBLE_ARCH_EVT_RECORD_WRITE_CMP:
		break;
	}
}


/*
 * @brief 	Get BLE mac address.
 * @param 	[out] mac: pointer to data
 * @return  MI_SUCCESS			The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note: 	You should copy gap mac to mac[6]  
 * */
mible_status_t mible_gap_address_get(mible_addr_t mac)
{
	uint32_t errno;
	ble_gap_addr_t gap_addr;
	errno = sd_ble_gap_address_get(&gap_addr);
	memcpy(mac, gap_addr.addr, 6);
	return (mible_status_t)errno;
}

/* GAP related function.  You should complement these functions. */

/*
 * @brief	Start scanning
 * @param 	[in] scan_type:	passive or active scaning
 * 			[in] scan_param: scan parameters including interval, windows
 * and timeout
 * @return  MI_SUCCESS             Successfully initiated scanning procedure.
 *          MI_ERR_INVALID_STATE   Has initiated scanning procedure.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * 		    MI_ERR_BUSY 		   The stack is busy, process pending
 * events and retry.
 * @note 	Other default scanning parameters : public address, no
 * whitelist.
 * 	        The scan response is given through
 * MIBLE_GAP_EVT_ADV_REPORT event
 */
mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
    mible_gap_scan_param_t scan_param)
{
    return MI_SUCCESS;
}

/*
 * @brief	Stop scanning
 * @param 	void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void) { return MI_SUCCESS; }

/*
 * @brief	Start advertising
 * @param 	[in] p_adv_param : pointer to advertising parameters, see
 * mible_gap_adv_param_t for details
 * @return  MI_SUCCESS             Successfully initiated advertising procedure.
 *          MI_ERR_INVALID_STATE   Initiated connectable advertising procedure
 * when connected.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 * retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again.
 * @note	Other default advertising parameters: local public address , no
 * filter policy
 * */
mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_adv_param)
{
	uint32_t errno;
	if (p_adv_param == NULL )
		return MI_ERR_INVALID_PARAM;

	errno = sd_ble_gap_adv_data_set(p_adv_param->adv_data, p_adv_param->adv_len, p_adv_param->scan_rsp_data, p_adv_param->scan_rsp_len);
    MI_ERR_CHECK(errno);

	ble_gap_adv_params_t adv_params = {0};
	adv_params.channel_mask.ch_37_off = p_adv_param->ch_mask.ch_37_off;
	adv_params.channel_mask.ch_38_off = p_adv_param->ch_mask.ch_38_off;
	adv_params.channel_mask.ch_39_off = p_adv_param->ch_mask.ch_39_off;
	adv_params.interval = p_adv_param->adv_interval_min;

	switch(p_adv_param->adv_type) {
		case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:
			adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
			break;

		case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:
			adv_params.type = BLE_GAP_ADV_TYPE_ADV_SCAN_IND;
			break;
			
		case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:
			adv_params.type = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
			break;
	}
	
	errno = sd_ble_gap_adv_start(&adv_params);
	MI_ERR_CHECK(errno);
	
    return MI_SUCCESS;
}
/*
 * @brief	Stop advertising
 * @param	void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void) {
	uint32_t errno;
	errno = sd_ble_gap_adv_stop();
	errno = errno == NRF_SUCCESS ? MI_SUCCESS : MI_ERR_INVALID_STATE;
	return (mible_status_t)errno;
}

/*
 * @brief  	Create a Direct connection
 * @param   [in] scan_param : scanning parameters, see TYPE
 * mible_gap_scan_param_t for details.
 * 			[in] conn_param : connection parameters, see TYPE
 * mible_gap_connect_t for details.
 * @return  MI_SUCCESS             Successfully initiated connection procedure.
 *          MI_ERR_INVALID_STATE   Initiated connection procedure in connected state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again
 *          MIBLE_ERR_GAP_INVALID_BLE_ADDR    Invalid Bluetooth address
 * supplied.
 * @note 	Own and peer address are both public.
 * 			The connection result is given by MIBLE_GAP_EVT_CONNECTED
 * event
 * */
mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
    mible_gap_connect_t conn_param)
{
    return MI_SUCCESS;
}

/*
 * @brief	Disconnect from peer
 * @param 	[in] conn_handle: the connection handle
 * @return  MI_SUCCESS             Successfully disconnected.
 *          MI_ERR_INVALID_STATE   Not in connnection.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note 	This function can be used by both central role and periphral
 * role.
 * */
mible_status_t mible_gap_disconnect(uint16_t conn_handle) {
	uint32_t errno;
	errno = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	errno = errno == NRF_SUCCESS ? MI_SUCCESS : MI_ERR_INVALID_STATE;
	return (mible_status_t)errno;
}

/*
 * @brief	Update the connection parameters.
 * @param  	[in] conn_handle: the connection handle.
 *			[in] conn_params: the connection parameters.
 * @return  MI_SUCCESS             The Connection Update procedure has been
 *started successfully.
 *          MI_ERR_INVALID_STATE   Initiated this procedure in disconnected
 *state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 *retry.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note  	This function can be used by both central role and peripheral
 *role.
 * */
mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
    mible_gap_conn_param_t conn_params)
{
	uint32_t errno;
	errno = sd_ble_gap_conn_param_update(conn_handle, (ble_gap_conn_params_t*)&conn_params);
	errno = errno == NRF_SUCCESS ? MI_SUCCESS : MI_ERR_INVALID_STATE;
	return (mible_status_t)errno;
}

/* GATTS related function  */

static ble_uuid_t convert_uuid(mible_uuid_t *p_uuid)
{
	uint32_t errno;
	ble_uuid_t uuid16 = {0};
	if (p_uuid->type == 0) {
		uuid16.type = 1;
		uuid16.uuid = p_uuid->uuid16;
	} else {
		ble_uuid128_t vs_uuid = {0};
		memcpy(vs_uuid.uuid128, p_uuid->uuid128, 16);
		errno = sd_ble_uuid_vs_add(&vs_uuid, &uuid16.type);
		MI_ERR_CHECK(errno);
		
		uuid16.uuid = vs_uuid.uuid128[13] << 8 | vs_uuid.uuid128[12];
	}
	
	return uuid16;
}

static uint32_t char_add(uint16_t                       service_handle,
	                     ble_uuid_t                     *p_uuid,
                         uint8_t                        *p_char_value,
                         uint16_t                        char_len,
                         ble_gatt_char_props_t           char_props,
                         uint16_t                       *p_value_handle,
	bool is_vlen, bool rd_auth, bool wr_auth)
{
	uint32_t errno;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_gatts_attr_md_t attr_md;
    ble_gatts_attr_md_t cccd_md;
	ble_gatts_char_handles_t handles;
    // The ble_gatts_attr_md_t structure uses bit fields. So we reset the memory to zero.
    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props = char_props;

    if (char_props.notify) {
		memset(&cccd_md, 0, sizeof(cccd_md));
		cccd_md.vloc         = BLE_GATTS_VLOC_STACK;
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
        char_md.p_cccd_md    = &cccd_md;
    } else {
        char_md.p_cccd_md    = NULL;
    }

    memset(&attr_md, 0, sizeof(attr_md));

	if (char_props.read) {
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
	}
	if (char_props.write || char_props.write_wo_resp) {
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
	}

    attr_md.vloc       = p_char_value == NULL ? BLE_GATTS_VLOC_STACK : BLE_GATTS_VLOC_USER;
    attr_md.rd_auth    = rd_auth;
    attr_md.wr_auth    = wr_auth;
    attr_md.vlen       = is_vlen;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = p_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.max_len   = char_len;
    attr_char_value.p_value   = p_char_value ? p_char_value : NULL;

    errno = sd_ble_gatts_characteristic_add(service_handle,
	                                       &char_md,
	                                       &attr_char_value,
	                                       &handles);

	*p_value_handle = handles.value_handle;
	return (mible_status_t)errno;
}

/*
 * @brief	Add a Service to a GATT server
 * @param 	[in|out] p_server_db: pointer to mible service data type 
 * of mible_gatts_db_t, see TYPE mible_gatts_db_t for details. 
 * @return  MI_SUCCESS             Successfully added a service declaration.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_NO_MEM	       Not enough memory to complete operation.
 * @note    This function can be implemented asynchronous. When service inition complete, call mible_arch_event_callback function and pass in MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP event and result.
 * */
mible_status_t mible_gatts_service_init(mible_gatts_db_t *p_server_db)
{
	uint32_t errno;
	ble_uuid_t uuid16 = {0};
	
	if (p_server_db == NULL)
		return MI_ERR_INVALID_ADDR;

	for (uint8_t idx = 0, max = p_server_db->srv_num; idx < max; idx++) {
		mible_gatts_srv_db_t service = p_server_db->p_srv_db[idx];

		// add vendor specific uuid
		uuid16 = convert_uuid(&service.srv_uuid);
		
		// add service
		errno = sd_ble_gatts_service_add(
			service.srv_type == MIBLE_PRIMARY_SERVICE ? BLE_GATTS_SRVC_TYPE_PRIMARY : BLE_GATTS_SRVC_TYPE_SECONDARY,
			&uuid16, &service.srv_handle);
		MI_ERR_CHECK(errno);

		// add charateristic
		for (uint8_t idx = 0, max = service.char_num; idx < max; idx++) {
			mible_gatts_char_db_t char_item = service.p_char_db[idx];
			ble_gatt_char_props_t props = {0};
			memcpy((uint8_t*)&props, &char_item.char_property, 1);
			uuid16 = convert_uuid(&char_item.char_uuid);
			errno = char_add(service.srv_handle, &uuid16, NULL, char_item.char_value_len,
					    props, &char_item.char_value_handle,
						char_item.is_variable_len, char_item.rd_author, char_item.wr_author);
			MI_ERR_CHECK(errno);
		}
	}

	return MI_SUCCESS;
}

/*
 * @brief	Set characteristic value
 * @param	[in] srv_handle: service handle
 *			[in] value_handle: characteristic value handle
 *			[in] offset: the offset from which the attribute value has
 *to be updated
 *			[in] p_value: pointer to data
 *			[in] len: data length
 * @return  MI_SUCCESS             Successfully retrieved the value of the
 *attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE  Attributes are not modifiable by
 *the application.
 * */
mible_status_t mible_gatts_value_set(uint16_t srv_handle, uint16_t value_handle,
    uint8_t offset, uint8_t* p_value,
    uint8_t len)
{
	uint32_t errno;
	ble_gatts_value_t value = {
		.len     = len,
		.offset  = offset,
		.p_value = p_value
	};
    errno = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, value_handle, &value);
	MI_ERR_CHECK(errno);
	return (mible_status_t)errno;
}

/*
 * @brief	Get charicteristic value as a GATTS.
 * @param 	[in] srv_handle: service handle
 * 			[in] value_handle: characteristic value handle
 *			[out] p_value: pointer to data which stores characteristic value
 * 			[out] p_len: pointer to data length.
 * @return  MI_SUCCESS             Successfully get the value of the attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 **/
mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t char_handle,
    uint8_t* p_value, uint8_t *p_len)
{
	uint32_t errno;
	ble_gatts_value_t value = {0};
    errno = sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, char_handle, &value);
	MI_ERR_CHECK(errno);
	memcpy(p_value, value.p_value, value.len);
	*p_len = value.len;
    return (mible_status_t)errno;
}

/*
 * @brief 	GATT Read or Write Authorize Reply parameters.
 * @param 	[in] conn_handle: conn handle
 *          [in] status_code: permitted = 1; not permitted = 2;
 *          [in] type : read = 1; write = 2;
 * 			[in] offset: the offset from which the attribute value has to
 * be updated
 * 			[in] len: data length
 * 			[in] pdata: pointer to data
 *
 * @return  MI_SUCCESS             Successfully queued a response to the peer, 
 * and in the case of a write operation, Attribute Table updated.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State or no authorization request pending.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE   Attributes are not modifiable by
 * the application.
 * */
mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle, uint8_t status_code,
	uint8_t type, uint16_t offset, uint16_t len, uint8_t *pdata)
{
	ble_gatts_rw_authorize_reply_params_t reply = {0};
	
	if (status_code == 1) {
		switch (type) {
		case 1:
			reply = (ble_gatts_rw_authorize_reply_params_t) {
				.type = BLE_GATTS_AUTHORIZE_TYPE_READ,
				.params.read.gatt_status = BLE_GATT_STATUS_SUCCESS,
				.params.read.p_data      = pdata,
				.params.read.len         = len
			};
			break;

		case 2:
			reply = (ble_gatts_rw_authorize_reply_params_t) {
				.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE,
				.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS,
				.params.write.update      = 1,
				.params.write.p_data      = pdata,
				.params.write.len         = len
			};
			break;
		}
	} else if (status_code == 2) {
		switch (type) {
		case 1:
			reply.params.read.gatt_status = BLE_GATT_STATUS_ATTERR_READ_NOT_PERMITTED;
			break;

		case 2:
			reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
			break;
		}
	}

	uint32_t errno = sd_ble_gatts_rw_authorize_reply(conn_handle, &reply);
	
	return (mible_status_t)errno;
}

// this function set char value and notify/indicate it to client
/*
 * @brief 	Set characteristic value and notify it to client.
 * @param 	[in] conn_handle: conn handle
 *          [in] srv_handle: service handle
 * 			[in] char_value_handle: characteristic handle
 * 			[in] offset: the offset from which the attribute value has to
 * be updated
 * 			[in] p_value: pointer to data
 * 			[in] len: data length
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
 * Configuration descriptor
 *          value to verify that the relevant operation (notification or
 * indication) has been enabled by the client.
 * */
mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
    uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
    uint8_t len, uint8_t type)
{
	uint32_t errno;
	uint16_t length = len;

	ble_gatts_hvx_params_t hvx = {
		.handle = char_value_handle,
		.type   = type,
		.offset = offset,
		.p_data = p_value,
		.p_len  = &length
	};
	errno = sd_ble_gatts_hvx(conn_handle, &hvx);
	MI_ERR_CHECK(errno);
    return MI_SUCCESS;
}



/* GATTC related function */

/*
 * @brief	Discover primary service by service UUID.
 * @param 	[in] conn_handle: connect handle
 * 			[in] handle_range: search range for primary sevice
 *discovery procedure
 * 			[in] uuid_type: 16-bit or 128-bit
 *			[in] p_srv_uuid: pointer to service uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Primary
 *Service Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note 	The response is given through
 *MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP event
 * */
mible_status_t mible_gattc_primary_service_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t* p_srv_uuid)
{
    /*tBleStatus errno;*/
    /*errno = aci_att_find_by_type_value_req(conn_handle,start_handle,end_handle,*/
    /*PRIMARY_SERVICE,2,svc_uuid);*/
    /*if( BLE_STATUS_SUCCESS != errno ){*/
    /*return MIBLE_ERR_UNKNOWN;*/
    /*}*/
    return MI_SUCCESS;
}

/*
 * @brief	Discover characteristic by characteristic UUID.
 * @param	[in] conn_handle: connect handle
 * 			[in] handle_range: search range for characteristic discovery
 * procedure
 * 			[in] uuid_type: 16-bit or 128-bit
 * 			[in] p_char_uuid: pointer to characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the
 * Characteristic Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note 	The response is given through
 * MIBLE_GATTC_CHR_DISCOVER_BY_UUID_RESP event
 * */
mible_status_t
mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t* p_char_uuid)
{
    return MI_SUCCESS;
}

/*
 * @brief	Discover characteristic client configuration descriptor
 * @param 	[in] conn_handle: connection handle
 * 			[in] handle_range: search range
 * @return  MI_SUCCESS             Successfully started Clien Config Descriptor
 * Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note	Maybe run the charicteristic descriptor discover procedure firstly,
 * then pick up the client configuration descriptor which att type is 0x2092
 * 			The response is given through MIBLE_GATTC_CCCD_DISCOVER_RESP
 * event
 * 			Only return the first cccd handle within the specified
 * range.
 * */
mible_status_t
mible_gattc_clt_cfg_descriptor_discover(uint16_t conn_handle,
    mible_handle_range_t handle_range)
{
    return MI_SUCCESS;
}

/*
 * @brief	Read characteristic value by UUID
 * @param 	[in] conn_handle: connnection handle
 * 			[in] handle_range: search range
 * 			[in] uuid_type: 16-bit or 128-bit
 * 			[in] char_uuid: characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Read
 * using Characteristic UUID procedure.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through
 * MIBLE_GATTC_EVT_READ_CHR_VALUE_BY_UUID_RESP event
 * */
mible_status_t
mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t *p_char_uuid)
{
    return MI_SUCCESS;
}

/*
 * @brief	Write characteristic value by handle
 * @param 	[in] conn_handle: connection handle
 * 			[in] handle: handle to the attribute to be written.
 * 			[in] p_value: pointer to data
 * 			[in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write with response
 * procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note  	The response is given through MIBLE_GATTC_EVT_WRITE_RESP event
 *
 * */
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle, uint16_t handle,
    uint8_t* p_value, uint8_t len)
{
    return MI_SUCCESS;
}

/*
 * @brief 	Write value by handle without response
 * @param   [in] conn_handle: connection handle
 * 			[in] att_handle: handle to the attribute to be written.
 * 			[in] p_value: pointer to data
 * 			[in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write Cmd procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note 	no response
 * */
mible_status_t mible_gattc_write_cmd(uint16_t conn_handle, uint16_t att_handle,
    uint8_t* p_value, uint8_t len)
{
    return MI_SUCCESS;
}

/*TIMER related function*/


/*
 * @brief 	Create a timer.
 * @param 	[out] p_timer_id: a pointer to timer id address which can uniquely identify the timer.
 * 			[in] timeout_handler: a pointer to a function which can be
 * called when the timer expires.
 * 			[in] mode: repeated or single shot.
 * @return  MI_SUCCESS             If the timer was successfully created.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   timer module has not been initialized or the
 * timer is running.
 *          MI_ERR_NO_MEM          timer pool is full.
 *
 * */


mible_status_t mible_timer_create(void** p_timer_id,
    mible_timer_handler timeout_handler,
    mible_timer_mode mode)
{
	uint32_t errno;
	
	app_timer_id_t id  = find_timer();
	if (id == NULL)
		return MI_ERR_NO_MEM;

	app_timer_mode_t m = mode == MIBLE_TIMER_SINGLE_SHOT ? APP_TIMER_MODE_SINGLE_SHOT : APP_TIMER_MODE_REPEATED;
	app_timer_timeout_handler_t handler = (void*)timeout_handler;
	errno = app_timer_create(&id, m, handler);
	MI_ERR_CHECK(errno);

	return (mible_status_t)errno;
}

/*
 * @brief 	Delete a timer.
 * @param 	[in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void * timer_id) 
{
	uint32_t errno;
	int id = free_timer(timer_id);
	if (id == -1)
		return MI_ERR_INVALID_PARAM;

	errno = app_timer_stop((app_timer_id_t)timer_id);
	return (mible_status_t)errno;
}

/*
 * @brief 	Start a timer.
 * @param 	[in] timer_id: timer id
 *          [in] timeout_value: Number of milliseconds to time-out event
 * (minimum 10 ms).
 * 			[in] p_context: parameters that can be passed to
 * timeout_handler
 *
 * @return  MI_SUCCESS             If the timer was successfully started.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   If the application timer module has not been
 * initialized or the timer has not been created.
 *         	MI_ERR_NO_MEM          If the timer operations queue was full.
 * @note 	If the timer has already started, it will start counting again.
 * */
mible_status_t mible_timer_start(void* timer_id, uint32_t timeout_value,
    void* p_context)
{
	uint32_t errno;
	errno = app_timer_start((app_timer_id_t)timer_id, APP_TIMER_TICKS(timeout_value, 0), p_context);
    return (mible_status_t)errno;
}

/*
 * @brief 	Stop a timer.
 * @param 	[in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully stopped.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *
 * */
mible_status_t mible_timer_stop(void* timer_id) 
{
	uint32_t errno;
	errno = app_timer_stop((app_timer_id_t)timer_id);
	return (mible_status_t)errno; 
}

/* FLASH related function*/

/*
 * @brief 	Create a record in flash 
 * @param 	[in] record_id: identify a record in flash 
 * 			[in] len: record length
 * @return 	MI_SUCCESS 				Create successfully.
 * 			MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *   		MI_ERR_NO_MEM,			Not enough flash memory to be assigned 
 * 				
 * */
mible_status_t mible_record_create(uint16_t record_id, uint8_t len)
{
	static uint8_t fds_has_init = 0;

	if (fds_has_init == 0) {
		fds_has_init = 1;
		mi_psm_init();
	}
	
	// actually it does not need record_id and len

	return MI_SUCCESS;	
}


/*
 * @brief  	Delete a record in flash
 * @param 	[in] record_id: identify a record in flash  
 * @return 	MI_SUCCESS 				Delete successfully. 
 * 			MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
	uint32_t errno;
	errno = mi_psm_record_delete(record_id);
	return (mible_status_t)errno;
}

/*
 * @brief 	Restore data to flash
 * @param 	[in] record_id: identify an area in flash
 * 			[out] p_data: pointer to data
 *			[in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data,
    uint8_t len)
{
	uint32_t errno;
	errno = mi_psm_record_read(record_id, p_data, len);
    return (mible_status_t)errno;
}

/*
 * @brief 	Store data to flash
 * @param 	[in] record_id: identify an area in flash
 * 			[in] p_data: pointer to data
 * 			[in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 * allowed size.
 *          MI_ERR_INVALID_PARAMS   p_data is not aligned to a 4 byte boundary.
 * @note  	Should use asynchronous mode to implement this function.
 *          The data to be written to flash has to be kept in memory until the
 * operation has terminated, i.e., an event is received.
 * 			When record writing complete , call mible_arch_event_callback function and pass MIBLE_ARCH_EVT_RECORD_WRITE_CMP event and result. 
 * */
mible_status_t mible_record_write(uint16_t record_id, uint8_t* p_data,
    uint8_t len)
{
	uint32_t errno;
	errno = mi_psm_record_write(record_id, p_data, len);
    return (mible_status_t)errno;
}

/*
 * @brief 	Get ture random bytes .
 * @param 	[out] p_buf: pointer to data
 * 			[in] len: Number of bytes to take from pool and place in
 * p_buff
 * @return  MI_SUCCESS			The requested bytes were written to
 * p_buff
 *          MI_ERR_NO_MEM       No bytes were written to the buffer, because
 * there were not enough random bytes available.
 * @note  	SHOULD use TRUE random num generator
 * */
mible_status_t mible_rand_num_generater(uint8_t* p_buf, uint8_t len)
{
	uint32_t errno;
	errno = sd_rand_application_vector_get(p_buf, len);
	return (mible_status_t)errno == MI_SUCCESS ? MI_SUCCESS : MI_ERR_NO_MEM;
}

/*
 * @brief 	Encrypts a block according to the specified parameters. 128-bit
 * AES encryption.
 * @param 	[in] key: encryption key
 * 			[in] plaintext: pointer to plain text
 * 			[in] plen: plain text length
 *          [out] ciphertext: pointer to cipher text
 * @return  MI_SUCCESS              The encryption operation completed.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE    Encryption module is not initialized.
 *          MI_ERR_INVALID_LENGTH   Length bigger than 16.
 *          MI_ERR_BUSY             Encryption module already in progress.
 * @note  	SHOULD use synchronous mode to implement this function
 * */
mible_status_t mible_ecb128_encrypt(const uint8_t* key,
    const uint8_t* plaintext, uint8_t plen,
    uint8_t* ciphertext)
{
	uint32_t errno;
	nrf_ecb_hal_data_t ctx = {0};

	if ( plaintext == NULL || key == NULL)
		return MI_ERR_INVALID_ADDR;
	else if (plen > 16)
		return MI_ERR_INVALID_LENGTH;

	memcpy(ctx.key, key, 16);
	memcpy(ctx.cleartext, plaintext, plen);
	errno = sd_ecb_block_encrypt(&ctx);
	memcpy(ciphertext, ctx.ciphertext, plen);

    return (mible_status_t)errno;
}

/*
 * @brief 	Post a task to a task quene, which can be executed in a right place(maybe a task in RTOS or while(1) in the main function).
 * @param 	[in] handler: a pointer to function 
 * 			[in] param: function parameters 
 * @return 	MI_SUCCESS 				Successfully put the handler to quene.		
 * 			MI_ERR_NO_MEM			The task quene is full. 
 * 			MI_ERR_INVALID_PARAM    Handler is NULL
 * */
mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
	uint32_t errno;
	if (handler == NULL)
		return MI_ERR_INVALID_PARAM;

	mible_task_t task = {.handler = handler, .arg = arg};
	errno = nrf_queue_push(&task_queue, &task);

	if (errno != NRF_SUCCESS)
		return MI_ERR_NO_MEM;
	else
		return MI_SUCCESS;
}

void mible_tasks_exec(void)
{
	uint32_t errno;
	mible_task_t task;
	errno = nrf_queue_pop(&task_queue, &task);
	MI_ERR_CHECK(errno);

	if (errno == NRF_SUCCESS)
		task.handler(task.arg);
}