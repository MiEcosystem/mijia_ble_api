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

#include "mible_arch.h"

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
		// mible_std_server_gap_evt_connected(param); 
        break;
    case MIBLE_GAP_EVT_DISCONNET:
		// mible_std_server_gap_evt_disconnected(param);
        break;
    case MIBLE_GAP_EVT_ADV_REPORT:
        // mible_std_server_gap_evt_scan_report(param); 
		break;
    case MIBLE_GAP_EVT_CONN_PARAM_UPDATED:
		// mible_std_server_gap_evt_conn_params_updated(param);
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
		// mible_std_server_gatts_evt_write(param);
        break;

    case MIBLE_GATTS_EVT_READ_PERMIT_REQ:
		// mible_std_server_gatts_evt_read_permit_req(param);
        break;

    case MIBLE_GATTS_EVT_WRITE_PERMIT_REQ:
		// mible_std_server_gatts_evt_write_permit_req(param);
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
	return MI_SUCCESS; 
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
mible_status_t mible_gap_scan_stop(void) 
{ 
	return MI_SUCCESS; 
}

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
    return MI_SUCCESS;
}
/*
 * @brief	Stop advertising
 * @param	void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void) 
{ 
	return MI_SUCCESS; 
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
mible_status_t mible_gap_disconnect(uint16_t conn_handle) 
{ 
	return MI_SUCCESS; 
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
    return MI_SUCCESS;
}

/* GATTS related function  */

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
    return MI_SUCCESS;
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
mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len)
{
    return MI_SUCCESS;
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
mible_status_t mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t *char_uuid)
{
    return MI_SUCCESS;
}

/*
 * @brief	Write value by handle with response
 * @param 	[in] conn_handle: connection handle
 * 			[in] att_handle: handle to the attribute to be written.
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
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle, uint16_t att_handle,
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
 * @param 	[out] p_timer_id: a pointer to timer id which can uniquely identify the timer.
 * 			[in] timeout_handler: a pointer to a function which can be
 * called when the timer expires.
 * 			[in] mode: repeated or single shot.
 * @return  MI_SUCCESS             If the timer was successfully created.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   timer module has not been initialized or the
 * timer is running.
 *
 * */
mible_status_t mible_timer_create(void* p_timer_id,
    mible_timer_handler timeout_handler,
    mible_timer_mode mode)
{
    return MI_SUCCESS;
}

/*
 * @brief 	Delete a timer.
 * @param 	[in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void* timer_id) 
{ 
	return MI_SUCCESS; 
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
    return MI_SUCCESS;
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
	return MI_SUCCESS; 
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
	return MI_SUCCESS;
}

/*
 * @brief 	Restore data to flash
 * @param 	[in] record_id: identify an area in flash
 * 			[out] p_data: pointer to data
 *			[out] p_len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data,
    uint8_t *p_len)
{
    return MI_SUCCESS;
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
    return MI_SUCCESS;
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
    return MI_SUCCESS;
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
 *          MI_ERR_INVALID_LENGTH    Invalid length supplied.
 *          MI_ERR_BUSY             Encryption module already in progress.
 * @note  	SHOULD use synchronous mode to implement this function
 * */
mible_status_t mible_ecb128_encrypt(const uint8_t* key,
    const uint8_t* plaintext, uint8_t plen,
    uint8_t* ciphertext)
{
    return MI_SUCCESS;
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
	return MI_SUCCESS;	
} 
