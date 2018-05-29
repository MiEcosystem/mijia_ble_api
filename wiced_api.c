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

#include "wiced_bt_types.h"
#include "wiced_memory.h"
#include "wiced_bt_ble.h"
#include "mible_log.h"
#include "wiced_timer.h"

#include "hci_control_api.h"
#include "wiced_bt_gatt.h"
#include "wiced_hal_rand.h"
#include "aes.h"
#include "Wiced_bt_l2c.h"
#include "wiced_transport.h"

#include "wiced_hal_nvram.h"
#include "queue.h"

#include "mible_wiced.h"

#ifndef MIBLE_MAX_USERS
#define MIBLE_MAX_USERS 4
#endif

/* GAP, GATTS, GATTC event callback function */
static uint8_t m_gap_users, m_gattc_users, m_gatts_users, m_arch_users;
static mible_gap_callback_t m_gap_cb_table[MIBLE_MAX_USERS];
static mible_gatts_callback_t m_gatts_cb_table[MIBLE_MAX_USERS];
static mible_gattc_callback_t m_gattc_cb_table[MIBLE_MAX_USERS];
static mible_arch_callback_t m_arch_cb_table[MIBLE_MAX_USERS];

#define MAX_TASK_NUM          4

typedef struct {
    mible_handler_t handler;
    void *arg;
} mible_task_t;

static mible_task_t task_buf[MAX_TASK_NUM];
static queue_t task_queue;

void mible_task_post_handler( uint32_t count )
{
	mible_task_t task;

	if (dequeue(&task_queue, &task) == MI_SUCCESS)
	{
	    //MI_LOG_INFO("\r\n****mible_task_post_handler: exectuing the post task\r\n");
		task.handler(task.arg);
	}
	else
	{
	   wiced_stop_timer( &mible_task_post_timer );
	   MI_LOG_INFO("mible_task_post_handler:stopped\r\n");
	}
}

int mible_gap_register(mible_gap_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_gap_users == MIBLE_MAX_USERS) {
        ret = MI_ERR_RESOURCES;
    } else {
        m_gap_cb_table[m_gap_users] = cb;
        m_gap_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return ret;
}

int mible_gattc_register(mible_gattc_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_gattc_users == MIBLE_MAX_USERS) {
        ret = MI_ERR_RESOURCES;
    } else {
        m_gattc_cb_table[m_gattc_users] = cb;
        m_gattc_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return ret;
}

int mible_gatts_register(mible_gatts_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_gatts_users == MIBLE_MAX_USERS) {
        ret = MI_ERR_RESOURCES;
    } else {
        m_gatts_cb_table[m_gatts_users] = cb;
        m_gatts_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return ret;
}

int mible_arch_register(mible_arch_callback_t cb)
{
    int ret;

    CRITICAL_SECTION_ENTER();
    if (m_arch_users == MIBLE_MAX_USERS) {
        ret = MI_ERR_RESOURCES;
    } else {
        m_arch_cb_table[m_arch_users] = cb;
        m_arch_users++;

        ret = MI_SUCCESS;
    }
    CRITICAL_SECTION_EXIT();

    return ret;
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
    int user = 0;
	MI_LOG_ERROR("mible_gap_event_callback+++\r\n");
    for (user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gap_cb_table[user] != NULL) {
            m_gap_cb_table[user](evt, param);
        }
    }
	MI_LOG_ERROR("mible_gap_event_callback---\r\n");
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
    int user = 0;
	MI_LOG_ERROR("mible_gatts_event_callback+++\r\n");
    for (user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gatts_cb_table[user] != NULL) {
            m_gatts_cb_table[user](evt, param);
        }
    }
	MI_LOG_ERROR("mible_gatts_event_callback---\r\n");
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
    int user = 0;
    for (user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gattc_cb_table[user] != NULL) {
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
    int user = 0;
    for (user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_arch_cb_table[user] != NULL) {
            m_arch_cb_table[user](evt, param);
        }
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
    wiced_bt_device_address_t bd_addr = { 0 };
    //uint8_t i;
    wiced_bt_dev_read_local_addr(bd_addr);
	memcpy(mac,bd_addr,6);
	MI_LOG_INFO("mible_gap_address_get,mac=%B\r\n",mac);

    return MI_SUCCESS;
}

void hci_control_le_send_advertisement_report( wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data )
{
    int       i;
    uint8_t   tx_buf[70];
    uint8_t   *p = tx_buf;
    uint8_t   len;

    *p++ = p_scan_result->ble_evt_type;
    *p++ = p_scan_result->ble_addr_type;
    for ( i = 0; i < 6; i++ )
        *p++ = p_scan_result->remote_bd_addr[5 - i];
    *p++ = p_scan_result->rssi;

    // currently callback does not pass the data of the adv data, need to go through the data
    // zero len in the LTV means that there is no more data
    while ( ( len = *p_adv_data ) != 0 )
    {
        for ( i = 0; i < len + 1; i++ )
            *p++ = *p_adv_data++;
    }
    wiced_transport_send_data ( HCI_CONTROL_LE_EVENT_ADVERTISEMENT_REPORT, tx_buf, ( int )( p - tx_buf ) );
}

void hci_control_le_scan_result_cback( wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data )
{
    if ( p_scan_result )
    {
        MI_LOG_INFO( "Device : %B \r\n", p_scan_result->remote_bd_addr );
        hci_control_le_send_advertisement_report( p_scan_result, p_adv_data );
    }
    else
    {
        MI_LOG_INFO( "Scan completed \r\n" );
    }
}

wiced_result_t hci_control_le_handle_scan_cmd(wiced_bool_t enable,
        wiced_bool_t filter_duplicates)
{
    wiced_result_t status;

    if (enable)
    {
        //memset(hci_control_le_cb.scanned_devices, 0, sizeof(hci_control_le_cb.scanned_devices));
        status = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, filter_duplicates,
                hci_control_le_scan_result_cback);
    }
    else
    {
        status = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_NONE, filter_duplicates,
                hci_control_le_scan_result_cback);
    }
    MI_LOG_INFO("hci_control_le_handle_scan_cmd:%d status:%x\r\n", enable, status);

    if ((status == WICED_BT_SUCCESS) ||
        (status == WICED_BT_PENDING))
    {
        return WICED_TRUE;
    }

    return WICED_FALSE;
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
	MI_LOG_INFO("mible_gap_scan_start()*****\r\n");

	if(hci_control_le_handle_scan_cmd(WICED_TRUE, WICED_TRUE))
		  return MI_SUCCESS;
	else
		return MIBLE_ERR_UNKNOWN;
}

/**
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void)
{
	MI_LOG_INFO("mible_gap_scan_stop()*****\r\n");
	//hci_control_le_send_scan_state_event( HCI_CONTROL_SCAN_EVENT_NO_SCAN );
	if(hci_control_le_handle_scan_cmd(WICED_FALSE, WICED_TRUE))
		  return MI_SUCCESS;
	else
		return MIBLE_ERR_UNKNOWN;
}

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
	wiced_bt_ble_advert_mode_t advert_mode=0;

    switch(p_param->adv_type) {
        case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:
			MI_LOG_INFO("mible_gap_adv_start: MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED\r\n");
            advert_mode= BTM_BLE_ADVERT_UNDIRECTED_LOW;
            break;

        case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:
			MI_LOG_INFO("mible_gap_adv_start: MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED\r\n");
            advert_mode = BTM_BLE_ADVERT_DISCOVERABLE_LOW;
            break;
            
        case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:
			MI_LOG_INFO("mible_gap_adv_start: MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED\r\n");
            advert_mode =BTM_BLE_ADVERT_NONCONN_LOW;
            break;
    }
	
    if(WICED_BT_SUCCESS != wiced_bt_start_advertisements(advert_mode, BLE_ADDR_PUBLIC, NULL))
    {
        MI_LOG_INFO("mible_gap_adv_start: MI_ERR_BUSY\r\n");
        return MI_ERR_BUSY;
    }
    MI_LOG_INFO("mible_gap_adv_start: MI_SUCCESS\r\n");
	
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
    uint16_t i=0;
	uint8_t num_elem = 0;
	uint8_t *pvRet[MIBLE_ADV_DATA_LENGTH] = {NULL};
	wiced_bt_ble_advert_elem_t adv_data[4];

	MI_LOG_INFO("adv: length=%d, data=", dlen);
	MI_LOG_HEXDUMP(p_data,dlen);	

    for(num_elem=0,i = 0; (i<dlen)&&(num_elem<MIBLE_ADV_DATA_LENGTH); i++)
    {
		adv_data[num_elem].len			= p_data[i]-1;
		adv_data[num_elem].advert_type	= p_data[i+1];
	    pvRet[num_elem] = (uint8_t*)wiced_bt_get_buffer(adv_data[num_elem].len);
	    if (pvRet[num_elem] == NULL)
	    {
	       	MI_LOG_INFO("Failed to allocate the memory\r\n");
			return MI_ERR_RESOURCES;   
	    }
		memset(pvRet[num_elem], 0, adv_data[num_elem].len);
		memcpy(pvRet[num_elem], &p_data[i+2], adv_data[num_elem].len);
		adv_data[num_elem].p_data = pvRet[num_elem];

		i=i+adv_data[num_elem].len+1;
		num_elem++;
    }
	
	for(i=0; i<num_elem; i++)
	{
		MI_LOG_INFO("adv_data[%d].advert_type=%d\r\n",i, adv_data[i].advert_type);
		MI_LOG_INFO("adv_data[%d].length=%d\r\n",i, adv_data[i].len);
		MI_LOG_INFO("adv_data[%d].data=", i);
		MI_LOG_HEXDUMP(adv_data[i].p_data,adv_data[i].len);
		MI_LOG_INFO("\r\n");
	}
	
	wiced_bt_ble_set_raw_advertisement_data(num_elem, adv_data);
	
	for(i=0; i<num_elem; i++)
	{
		if(pvRet[i])
		{
			wiced_bt_free_buffer(pvRet[i]);
			pvRet[i]=NULL;
		}
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
	wiced_result_t status;

    status = wiced_bt_start_advertisements( BTM_BLE_ADVERT_OFF, BLE_ADDR_PUBLIC, NULL );
    if (status != WICED_BT_SUCCESS)
    {
        MI_LOG_INFO("mible_gap_adv_stop,MIBLE_ERR_UNKNOWN\r\n");
        return MIBLE_ERR_UNKNOWN;
    }
    MI_LOG_INFO("mible_gap_adv_stop: MI_SUCCESS\r\n");
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

mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
        mible_gap_connect_t conn_param)
{
	int i=0;
	wiced_bt_ble_address_type_t addr_type;
	wiced_bool_t status;
	wiced_bt_device_address_t bd_addr;
	uint8_t status1=0;

	MI_LOG_INFO("mible_gap_connect+++\r\n");

	switch (conn_param.type) {
	case MIBLE_ADDRESS_TYPE_PUBLIC:
	    addr_type = BLE_ADDR_PUBLIC;
	    break;
	case MIBLE_ADDRESS_TYPE_RANDOM:
	    addr_type = BLE_ADDR_RANDOM;
	    break;
    }
	memcpy(bd_addr, conn_param.peer_addr, 6);
    status = wiced_bt_gatt_le_connect(bd_addr,addr_type,BLE_CONN_MODE_HIGH_DUTY, WICED_TRUE );

    MI_LOG_INFO("mible_gap_connect status %d, BDA %B, Addr Type %x\r\n",status, bd_addr, addr_type );

	if(status == WICED_TRUE )
		status1=HCI_CONTROL_STATUS_SUCCESS;
	else
		status1=HCI_CONTROL_STATUS_FAILED;

	wiced_transport_send_data( HCI_CONTROL_GATT_EVENT_COMMAND_STATUS, &status1, 1 );

	if(status == WICED_TRUE)
		return MI_SUCCESS;
	else
		return MIBLE_ERR_UNKNOWN;
	
	MI_LOG_INFO("mible_gap_connect---\r\n");

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
    if (WICED_BT_GATT_SUCCESS!=wiced_bt_gatt_disconnect (conn_handle))
    {
        MI_LOG_INFO("mible_gap_disconnect_MIBLE_ERR_UNKNOWN\r\n");
        return MIBLE_ERR_UNKNOWN ;
    }
    MI_LOG_INFO("mible_gap_disconnect_success\r\n");
	
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
    wiced_bool_t status;

    MI_LOG_INFO("mible_gap_update_conn_params, MAC=%B\r\n",host_addr);
	MI_LOG_INFO("mible_gap_update_conn_params: min=%d, max=%d, latency=%d, timeout=%d\r\n",
		conn_params.min_conn_interval, conn_params.max_conn_interval,conn_params.slave_latency, conn_params.conn_sup_timeout);
	
    status = wiced_bt_l2cap_enable_update_ble_conn_params(host_addr, WICED_TRUE);
    if (status != WICED_TRUE)
    {
        MI_LOG_INFO("wiced_bt_l2cap_enable_update_ble_conn_params is failed\r\n");
        return MIBLE_ERR_UNKNOWN;
    }
	
    status = wiced_bt_l2cap_update_ble_conn_params(host_addr, conn_params.min_conn_interval, conn_params.max_conn_interval, 
		conn_params.slave_latency, conn_params.conn_sup_timeout);
    if (status != WICED_TRUE)
    {
        MI_LOG_INFO("mible_gap_update_conn_params: Failed\r\n");
        return MIBLE_ERR_UNKNOWN;
    }
	MI_LOG_INFO("mible_gap_update_conn_params: MI_SUCCESS \r\n");
	
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
	mible_arch_evt_param_t param;
	uint8_t i,j,cnt;
	
	if(p_server_db==NULL)
	{
		return MI_ERR_INVALID_ADDR;
	}
	param.srv_init_cmp.p_gatts_db=p_server_db;

	MI_LOG_INFO("mible_gatts_service_init, service uuid=0x%02x****\r\n", p_server_db->p_srv_db->srv_uuid.uuid16);
	
	if((p_server_db->p_srv_db->srv_uuid.uuid16) == UUID_SERVICE_SQUIRREL)
	{
	   ( param.srv_init_cmp.p_gatts_db->p_srv_db->srv_handle)=SQUIRREL_IDX_SVC;
	}

	for(i=0;i<p_server_db->p_srv_db->char_num;i++)
	{
	   MI_LOG_INFO("mible_gatts_service_init, char uuid=%02x\r\n",p_server_db->p_srv_db->p_char_db[i].char_uuid.uuid16);
	   for(j=0;j<MIBLE_GATT_HANDLE_MAX_NUM;j++)
	   {
		   if((p_server_db->p_srv_db->p_char_db[i].char_uuid.uuid16==gMibleGatt_db[j].handle_uuid) )
		   {
			   param.srv_init_cmp.p_gatts_db->p_srv_db->p_char_db[i].char_value_handle=gMibleGatt_db[j].handle_val;
			   mible_gatts_value_set(gMibleGatt_db[j].service_handle,gMibleGatt_db[j].handle_val,0,(p_server_db->p_srv_db->p_char_db[i].p_value),(p_server_db->p_srv_db->p_char_db[i].char_value_len));
			   //MI_LOG_INFO("mible_gatts_service_init handle");
			   break;
		   }
	   }
	  if(j==MIBLE_GATT_HANDLE_MAX_NUM)
	   {
		   cnt++;
		   MI_LOG_INFO("mible_gatts_service_init NOT FIND VALID HANDLE\r\n");
	   }
	}
	if(cnt==p_server_db->p_srv_db->char_num)
	{
		param.srv_init_cmp.status=MI_ERR_INVALID_PARAM;
		return MI_ERR_INVALID_PARAM;
	}
	else
	{
		param.srv_init_cmp.status=MI_SUCCESS;
		mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
		//mible_std_arch_event_handler(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
		//MI_LOG_INFO("mible_gatts_service_init: MI_SUCCESS\r\n");
		return MI_SUCCESS;
	}
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
mible_status_t mible_gatts_value_set(uint16_t srv_handle, uint16_t value_handle, uint8_t offset, uint8_t* p_value, uint8_t len)
{
    uint8_t i=0;

    for(i=0;i<MIBLE_GATT_HANDLE_MAX_NUM;i++)
    {
        if((srv_handle==gMibleGatt_db[i].service_handle ) && (value_handle==gMibleGatt_db[i].handle_val))
        {
            //MI_LOG_INFO("mible_gatts_value_set, locating the handle\r\n");
            break;
        }
    }

    if(offset > gMibleGatt_db[i].handle_max_len)
    {
        MI_LOG_INFO("mible_gatts_value_set: MI_ERR_INVALID_PARAM\r\n");
        return MI_ERR_INVALID_PARAM;
    }
    else if(len > gMibleGatt_db[i].handle_max_len)
    {
        MI_LOG_INFO("mible_gatts_value_set: MI_ERR_INVALID_LENGTH2\r\n");
        return MI_ERR_INVALID_LENGTH;
    }
    else
    {
        gMibleGatt_db[i].offset=offset;
        gMibleGatt_db[i].ucDataLen=len;
		gMibleGatt_db[i].handle_max_len=len;
        memcpy(gMibleGatt_db[i].pUcData+offset,p_value,len);
     
		MI_LOG_INFO("mible_gatts_value_set: index=%d, length=%d, data=",i,gMibleGatt_db[i].ucDataLen);
	    MI_LOG_HEXDUMP((gMibleGatt_db[i].pUcData+offset),gMibleGatt_db[i].ucDataLen);
		
        //MI_LOG_INFO("mible_gatts_value_set: MI_SUCCESS\r\n");
        return MI_SUCCESS;
    }

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
 mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t value_handle, uint8_t* p_value, uint8_t *p_len)
{
    uint8_t i=0;

    for(i=0;i<MIBLE_GATT_HANDLE_MAX_NUM;i++)
    {
        if((srv_handle==gMibleGatt_db[i].service_handle ) && (value_handle==gMibleGatt_db[i].handle_val))
        {
            //MI_LOG_INFO("mible_gatts_value_get_get handle\r\n");
            break;
        }
    }

    if( *p_len > gMibleGatt_db[i].handle_max_len)
    {
        *p_len=gMibleGatt_db[i].handle_max_len;
    }
    memcpy(p_value,gMibleGatt_db[i].pUcData,*p_len);

    MI_LOG_INFO("mible_gatts_value_get: MI_SUCCESS\r\n");
	
    return MI_SUCCESS;
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
    wiced_result_t status;
    wiced_bt_gatt_status_t bt_gatt_status;
    uint8_t  i=0;
	
    MI_LOG_INFO("mible_gatts_notify_or_indicate: char_value_handle=0x%02x,srv_handle=0x%02x\r\n",char_value_handle,srv_handle);
	
   for(i=0;i<MIBLE_GATT_HANDLE_MAX_NUM; i++)
   {
        if((srv_handle ==gMibleGatt_db[i].service_handle) && (char_value_handle==gMibleGatt_db[i].handle_val))
        {
            //MI_LOG_INFO("mible_gatts_notify_or_indicate_get handle");
            break;
        }
    }

    if(offset > gMibleGatt_db[i].handle_max_len)
    {
        MI_LOG_INFO("mible_gatts_notify_or_indicate: MI_ERR_INVALID_PARAM\r\n");
        return MI_ERR_INVALID_PARAM;
    }
    else if(len > gMibleGatt_db[i].handle_max_len)
    {
        MI_LOG_INFO("mible_gatts_notify_or_indicate: MI_ERR_INVALID_LENGTH\r\n");
        return MI_ERR_INVALID_LENGTH;
    }
    else
    {
        if (type == 1)
        {
            //sent notification
            MI_LOG_INFO("mible_gatts_notify\r\n");
           bt_gatt_status = wiced_bt_gatt_send_notification (conn_handle,gMibleGatt_db[i].handle_val , len, p_value );

        }
        else if(type == 2)
        {
            // send indication
            MI_LOG_INFO("mible_gatts_indication\r\n");
            bt_gatt_status =wiced_bt_gatt_send_indication(conn_handle, gMibleGatt_db[i].handle_val, len, p_value);
        }
        else
        {
            ;//do nothing;
        }

        if (bt_gatt_status != WICED_BT_GATT_SUCCESS)
        {
            MI_LOG_INFO("mible_gatts_notify_or_indicate: MI_ERR_INVALID_STATE\r\n");
            return MI_ERR_INVALID_STATE;
        }
        MI_LOG_INFO("mible_gatts_notify_or_indicate: MI_SUCCESS\r\n");
		
        return MI_SUCCESS;
    }

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
 * @brief   Discover primary service by service UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for primary sevice
 *discovery procedure
 *          [in] p_srv_uuid: pointer to service uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Primary
 *Service Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    The response is given through
 *MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP event
 * */
__WEAK mible_status_t mible_gattc_primary_service_discover_by_uuid(
        uint16_t conn_handle, mible_handle_range_t handle_range,
        mible_uuid_t* p_srv_uuid)
{

    return MI_SUCCESS;
}

/**
 * @brief   Discover characteristic by characteristic UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for characteristic discovery
 * procedure
 *          [in] p_char_uuid: pointer to characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the
 * Characteristic Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through
 * MIBLE_GATTC_CHR_DISCOVER_BY_UUID_RESP event
 * */
__WEAK mible_status_t mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
        mible_handle_range_t handle_range, mible_uuid_t* p_char_uuid)
{
    return MI_SUCCESS;
}

/**
 * @brief   Discover characteristic client configuration descriptor
 * @param   [in] conn_handle: connection handle
 *          [in] handle_range: search range
 * @return  MI_SUCCESS             Successfully started Clien Config Descriptor
 * Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    Maybe run the charicteristic descriptor discover procedure firstly,
 * then pick up the client configuration descriptor which att type is 0x2092
 *          The response is given through MIBLE_GATTC_CCCD_DISCOVER_RESP
 * event
 *          Only return the first cccd handle within the specified
 * range.
 * */
__WEAK mible_status_t mible_gattc_clt_cfg_descriptor_discover(
        uint16_t conn_handle, mible_handle_range_t handle_range)
{
    return MI_SUCCESS;
}

/**
 * @brief   Read characteristic value by UUID
 * @param   [in] conn_handle: connnection handle
 *          [in] handle_range: search range
 *          [in] p_char_uuid: pointer to characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Read
 * using Characteristic UUID procedure.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through
 * MIBLE_GATTC_EVT_READ_CHR_VALUE_BY_UUID_RESP event
 * */
__WEAK mible_status_t mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
        mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid)
{
    return MI_SUCCESS;
}

/**
 * @brief   Write value by handle with response
 * @param   [in] conn_handle: connection handle
 *          [in] handle: handle to the attribute to be written.
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
__WEAK mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle,
        uint16_t att_handle, uint8_t* p_value, uint8_t len)
{
    return MI_SUCCESS;
}

/**
 * @brief   Write value by handle without response
 * @param   [in] conn_handle: connection handle
 *          [in] att_handle: handle to the attribute to be written.
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
__WEAK mible_status_t mible_gattc_write_cmd(uint16_t conn_handle,
        uint16_t att_handle, uint8_t* p_value, uint8_t len)
{
    return MI_SUCCESS;
}

/**
 *        SOFT TIMER APIs
 */

/**
 * @brief   Create a timer.
 * @param   [out] p_timer_id: a pointer to timer id address which can uniquely identify the timer.
 *          [in] timeout_handler: a pointer to a function which can be
 * called when the timer expires.
 *          [in] mode: repeated or single shot.
 * @return  MI_SUCCESS             If the timer was successfully created.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   timer module has not been initialized or the
 * timer is running.
 *          MI_ERR_NO_MEM          timer pool is full.
 *
 * */
  mible_status_t mible_timer_create(void** p_timer_id,
        mible_timer_handler timeout_handler, mible_timer_mode mode)
{
	uint32_t i= *(uint32_t *)p_timer_id;

	//MI_LOG_INFO("mible_timer_create: *(uint32_t *)p_timer_id=%d\r\n",*(uint32_t *)p_timer_id);

	if (NULL == p_timer_id)
	{
	    MI_LOG_INFO("mible_timer_create: MI_ERR_INVALID_PARAM");
	    return MI_ERR_INVALID_PARAM;
	}
	
	if( (i>0) && (i<= MIBLE_TIMER_MAX_NUM))
	{
		if(gMiJia_TimerPool[i-1].created)
		{
			MI_LOG_INFO("mible_timer_create: the time was created before\r\n");
			return MI_ERR_INVALID_STATE;
		}
	}

	for (i = 0; i <= MIBLE_TIMER_MAX_NUM; i++)
	{
		if(gMiJia_TimerPool[i].created!=1)
		{
		    *(uint32_t *)p_timer_id = i+1;
			gMiJia_TimerPool[i].pFunc = (wiced_timer_callback_t *)timeout_handler;
			gMiJia_TimerPool[i].mode=mode;
			gMiJia_TimerPool[i].created=1;
			break;
		}
	}
	MI_LOG_INFO("mible_timer_created: successful, index=%d\r\n",i);
	
	return MI_SUCCESS;
							 
}

/**
 * @brief   Delete a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void* timer_id)
{
	//uint32_t timer_index = *(uint32_t *)timer_id;
	uint32_t timer_index = (uint32_t )timer_id;
	
	MI_LOG_INFO("mible_timer_delete, timer_id=%d\r\n",(uint32_t)timer_id);

    if ((timer_index > MIBLE_TIMER_MAX_NUM) || (timer_index == 0))
    {
        MI_LOG_INFO("mible_timer_delete: MI_ERR_INVALID_PARAM\r\n");
        return MI_ERR_INVALID_PARAM;
    }
	
	timer_index=timer_index-1;

    if (gMiJia_TimerPool[timer_index].created)
    {
        wiced_deinit_timer(&gMiJia_TimerPool[timer_index].timer);
    }
    memset(&gMiJia_TimerPool[timer_index], 0, sizeof(Mijia_Timer_db));
	
    MI_LOG_INFO("mible_timer_delete: MI_SUCCESS\r\n");
	
    return MI_SUCCESS;

}

/**
 * @brief   Start a timer.
 * @param   [in] timer_id: timer id
 *          [in] timeout_value: Number of milliseconds to time-out event
 * (minimum 10 ms).
 *          [in] p_context: parameters that can be passed to
 * timeout_handler
 *
 * @return  MI_SUCCESS             If the timer was successfully started.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   If the application timer module has not been
 * initialized or the timer has not been created.
 *          MI_ERR_NO_MEM          If the timer operations queue was full.
 * @note    If the timer has already started, it will start counting again.
 * */
mible_status_t mible_timer_start(void* timer_id, uint32_t timeout_value,
        void* p_context)
{
	uint32_t timer_index = (uint32_t)timer_id;
	uint32_t *index = (uint32_t *)timer_id;
	wiced_timer_type_t xTimerType = WICED_MILLI_SECONDS_TIMER;

	//MI_LOG_INFO("mible_timer_start, (uint32_t *)timer_id=%d\r\n",(uint32_t *)timer_id);
	MI_LOG_INFO("mible_timer_start, index=%d\r\n",index);


	if ((timer_index>MIBLE_TIMER_MAX_NUM) || (timer_index== 0))
    {
        MI_LOG_INFO("mible_timer_start: MI_ERR_INVALID_PARAM\r\n");
        return MI_ERR_INVALID_PARAM;
    }
	
	timer_index = timer_index - 1;
	
	if (MIBLE_TIMER_REPEATED == gMiJia_TimerPool[timer_index].mode)
    {
        xTimerType = WICED_MILLI_SECONDS_PERIODIC_TIMER;
    }

	if (MI_SUCCESS == mible_timer_stop((void *)timer_index))
    //if (MI_SUCCESS == mible_timer_stop(timer_id)) 
    {
        wiced_deinit_timer(&gMiJia_TimerPool[timer_index].timer);
         gMiJia_TimerPool[timer_index].created = 0; 
    }/**/

    if (WICED_BT_SUCCESS != wiced_init_timer(&gMiJia_TimerPool[timer_index].timer,
                             (wiced_timer_callback_t)gMiJia_TimerPool[timer_index].pFunc, (uint32_t)p_context, xTimerType))
    {
        MI_LOG_INFO("mible_timer_start: MI_ERR_NO_MEM\r\n");
        return MI_ERR_NO_MEM;
    }
     gMiJia_TimerPool[timer_index].created = 1; 
  
    if (WICED_BT_SUCCESS != wiced_start_timer(&gMiJia_TimerPool[timer_index].timer, timeout_value))
    {
        return MI_ERR_NO_MEM;
    }
	
	MI_LOG_INFO("mible_timer_started, timer_id=%d\r\n",timer_id);

	return MI_SUCCESS;

}

/**
 * @brief   Stop a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully stopped.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *
 * */
mible_status_t mible_timer_stop(void* timer_id)
{
	uint32_t timer_index = (uint32_t )timer_id;

	MI_LOG_INFO("mible_timer_stop: timer_id=%d\r\n",(uint32_t)timer_id);

	if (timer_index > MIBLE_TIMER_MAX_NUM || timer_index == 0)
	{
		MI_LOG_INFO("mible_timer_stop: MI_ERR_INVALID_PARAM\r\n");
		return MI_ERR_INVALID_PARAM;
	}
	
	timer_index = timer_index - 1;
	
	if (0 == gMiJia_TimerPool[timer_index].created)
	{
		MI_LOG_INFO("mible_timer_stop: not created\r\n");
		return MI_ERR_INVALID_PARAM;
	}
	if (WICED_BT_SUCCESS != wiced_stop_timer(&gMiJia_TimerPool[timer_index].timer))
	{
		MI_LOG_INFO("mible_timer_stop: MI_ERR_INVALID_PARAM\r\n");
		return MI_ERR_INVALID_PARAM;
	}

	MI_LOG_INFO("mible_timer_stop: MI_SUCCESS\r\n");
	
    return MI_SUCCESS;

}
/**
 *        NVM APIs
 */

/**
 * @brief   Create a record in flash 
 * @param   [in] record_id: identify a record in flash 
 *          [in] len: record length
 * @return  MI_SUCCESS              Create successfully.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_NO_MEM,          Not enough flash memory to be assigned 
 *              
 * */
mible_status_t mible_record_create(uint16_t record_id, uint8_t len)
{
    //Cypress SDK don't need to create the record, and can write directly. 
	MI_LOG_INFO("mible_record_create, return success, record_id=%d, len=%d\r\n", record_id, len);

	return MI_SUCCESS;
}

/**
 * @brief   Delete a record in flash
 * @param   [in] record_id: identify a record in flash  
 * @return  MI_SUCCESS              Delete successfully. 
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
	uint8_t vs_id=0;
	wiced_result_t result;

	if(record_id==MIBLE_DID_RECORD_ID)
		vs_id=MI_BLE_DID_VS_ID;
	else if(record_id==MIBLE_TOKEN_RECORD_ID)	
		vs_id=MI_BLE_TOKEN_VS_ID;
	else if(record_id==MIBLE_BEACONKEY_RECORD_ID)
		vs_id=MI_BLE_BEACONKEY_VS_ID;
	else
	{
		MI_LOG_INFO("mible_record_delete: ID Not Found, record_id=%d\r\n", record_id);
		return MI_ERR_NOT_FOUND;
	}
	
	wiced_hal_delete_nvram(vs_id, &result);
	if(result != WICED_SUCCESS)
	{
		MI_LOG_INFO("mible_record_delete:Failed, code=%d\r\n", result);
		return MI_ERR_NOT_FOUND;
	}

	MI_LOG_INFO("******mible_record_delete: Successful, deleted_id=%d\r\n",vs_id);

	return MI_SUCCESS;
}

/**
 * @brief   Restore data to flash
 * @param   [in] record_id: identify an area in flash
 *          [out] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data,
        uint8_t len)
{
	uint8_t vs_id=0;
	wiced_result_t result;
	uint8_t read_bytes = 0;
	
	MI_LOG_INFO("mible_record_read: reading, record_id=%d, len=%d\r\n",record_id,len);

	if(record_id==MIBLE_DID_RECORD_ID)
		vs_id=MI_BLE_DID_VS_ID;
	else if(record_id==MIBLE_TOKEN_RECORD_ID)	
		vs_id=MI_BLE_TOKEN_VS_ID;
	else if(record_id==MIBLE_BEACONKEY_RECORD_ID)
		vs_id=MI_BLE_BEACONKEY_VS_ID;
	else
	{
		MI_LOG_INFO("mible_record_read: ID Not Found, record_id=%d\r\n", record_id);
		return MI_ERR_NOT_FOUND;
	}

	read_bytes=wiced_hal_read_nvram( vs_id, len, p_data, &result );
 	if(result != WICED_SUCCESS)
 	{
 		MI_LOG_INFO("mible_record_read: Failed, code=%d\r\n",result);
		return MI_ERR_NOT_FOUND;
 	}
	MI_LOG_INFO("mible_record_read: Successful, record_id=%d, len=%d, data=\r\n",record_id,len);
	MI_LOG_HEXDUMP(p_data,  (uint16_t)len);

	return MI_SUCCESS;
}

/**
 * @brief   Store data to flash
 * @param   [in] record_id: identify an area in flash
 *          [in] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 * allowed size.
 *          MI_ERR_INVALID_PARAMS   p_data is not aligned to a 4 byte boundary.
 * @note    Should use asynchronous mode to implement this function.
 *          The data to be written to flash has to be kept in memory until the
 * operation has terminated, i.e., an event is received.
 *          When record writing complete , call mible_arch_event_callback function and pass MIBLE_ARCH_EVT_RECORD_WRITE_CMP event and result. 
 * */
mible_status_t mible_record_write(uint16_t record_id, uint8_t* p_data,
        uint8_t len)
{
	uint8_t vs_id=0;
	wiced_result_t result;
	uint8_t written_byte = 0;
	
	MI_LOG_INFO("mible_record_write,record_id=%d, len=%d, data=\r\n",record_id, len);
	MI_LOG_HEXDUMP(p_data,  (uint16_t)len);

	if(record_id==MIBLE_DID_RECORD_ID)
		vs_id=MI_BLE_DID_VS_ID;
	else if(record_id==MIBLE_TOKEN_RECORD_ID)	
		vs_id=MI_BLE_TOKEN_VS_ID;
	else if(record_id==MIBLE_BEACONKEY_RECORD_ID)
		vs_id=MI_BLE_BEACONKEY_VS_ID;
	else
	{
		MI_LOG_INFO("mible_record_write: ID Not Found, record_id=%d\r\n", record_id);
		return MI_ERR_NOT_FOUND;
	}

	written_byte = wiced_hal_write_nvram_patch(vs_id, len, p_data, &result );
	if(result == WICED_SUCCESS)
	{
		MI_LOG_INFO("mible_record_write: Successful, record_id=%d, write_bytes=%d, \r\n", record_id, written_byte);
		return MI_SUCCESS;
	}
	MI_LOG_INFO("mible_record_write: Failed, code=%d \r\n",result);

    return MI_ERR_RESOURCES;

}

/**
 *        MISC APIs
 */

/**
 * @brief   Get ture random bytes .
 * @param   [out] p_buf: pointer to data
 *          [in] len: Number of bytes to take from pool and place in
 * p_buff
 * @return  MI_SUCCESS          The requested bytes were written to
 * p_buff
 *          MI_ERR_NO_MEM       No bytes were written to the buffer, because
 * there were not enough random bytes available.
 * @note    SHOULD use TRUE random num generator
 * */
mible_status_t mible_rand_num_generator(uint8_t* p_buf, uint8_t len)
{
     uint8_t i,j;
     uint8_t *tempBuf=(uint8_t*)wiced_memory_permanent_allocate(len);
     uint32_t tempData;
     if(len<4)
     {
       tempData=wiced_hal_rand_gen_num();
       for(i=0;i<len;i++)
       {
           tempBuf[i] = (tempData >>(8*i))& 0xFF;
       }
       memcpy(p_buf,tempBuf,len);
       wiced_bt_free_buffer(tempBuf);
     }
     else
     {
         for(i=0;i<(len-(len%4));i=i+4)
         {
             tempData= wiced_hal_rand_gen_num();
             tempBuf[i] = tempData &0xFF;
             tempBuf[i+1] = (tempData >>8) &0xFF;
             tempBuf[i+2] =(tempData >>16)& 0xFF;
             tempBuf[i+3] = (tempData >>24)& 0xFF;
         }
         tempData= wiced_hal_rand_gen_num();
         for(j=0;j<(len%4);j++)
         {
             tempBuf[i] = (tempData >>(8*i))& 0xFF;
         }
         memcpy(p_buf,tempBuf,len);
         wiced_bt_free_buffer(tempBuf);
     }

	 MI_LOG_INFO("mible_rand_num_generator: p_buf[len=%d]",i);
	 MI_LOG_HEXDUMP(p_buf, len);
	 
     return MI_SUCCESS;
}

/**
 * @brief   Encrypts a block according to the specified parameters. 128-bit
 * AES encryption. (zero padding)
 * @param   [in] key: encryption key
 *          [in] plaintext: pointer to plain text
 *          [in] plen: plain text length
 *          [out] ciphertext: pointer to cipher text
 * @return  MI_SUCCESS              The encryption operation completed.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE    Encryption module is not initialized.
 *          MI_ERR_INVALID_LENGTH   Length bigger than 16.
 *          MI_ERR_BUSY             Encryption module already in progress.
 * @note    SHOULD use synchronous mode to implement this function
 * */

mible_status_t mible_aes128_encrypt(const uint8_t* key,
        const uint8_t* plaintext, uint8_t plen, uint8_t* ciphertext)
{
    struct AES_ctx ctx;
    uint8_t plainData[16]={0};
    uint8_t i;
    if((key==NULL) || (plaintext==NULL))
    {
       return  MI_ERR_INVALID_ADDR;
    }
    if(plen > 16)
    {
        return MI_ERR_INVALID_LENGTH;
    }
    AES_init_ctx(&ctx, key);
    if(plen==16)
    {
        AES_ECB_encrypt(&ctx, plaintext);
        memcpy(ciphertext,plaintext,plen);
    }
    else
    {
        memcpy((uint8_t *)plainData,plaintext,plen);
        AES_ECB_encrypt(&ctx, plainData);
        memcpy(ciphertext,plainData,plen);
    }

	//MI_LOG_HEXDUMP(ciphertext,16);
	
    return MI_SUCCESS;
}

/**
 * @brief   Post a task to a task quene, which can be executed in a right place 
 * (maybe a task in RTOS or while(1) in the main function).
 * @param   [in] handler: a pointer to function 
 *          [in] param: function parameters 
 * @return  MI_SUCCESS              Successfully put the handler to quene.
 *          MI_ERR_NO_MEM           The task quene is full. 
 *          MI_ERR_INVALID_PARAM    Handler is NULL
 * */

 mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
	mible_task_t task;
	
	MI_LOG_INFO("mible_task_post\r\n");

	if(!wiced_is_timer_in_use(&mible_task_post_timer))
	{
		if(WICED_BT_SUCCESS== wiced_start_timer(&mible_task_post_timer,TASK_POST_TIMEOUT))
		{
			MI_LOG_INFO("mible_task_post,start_timer: Successful\r\n");
		}
	}
	else
		MI_LOG_INFO("mible_task_post,start_timer: using\r\n");
	
    task.handler = handler;
    task.arg     = arg;
	//MI_LOG_INFO("mible_task_post,start_timer: posting\r\n");

    return enqueue(&task_queue, &task);
}

/**
 * @brief   Function for executing all enqueued tasks.
 *
 * @note    This function must be called from within the main loop. It will 
 * execute all events scheduled since the last time it was called.
 * */
 __WEAK void mible_tasks_exec(void)
{
	MI_LOG_INFO("mible_tasks_exec, this API is not done!!!\r\n");
}

/**
 *        IIC APIs
 */

/**
 * @brief   Function for initializing the IIC driver instance.
 * @param   [in] p_config: Pointer to the initial configuration.
 *          [in] handler: Event handler provided by the user. 
 * @return  MI_SUCCESS              Initialized successfully.
 *          MI_ERR_INVALID_PARAM    p_config or handler is a NULL pointer.
 *              
 * */
__WEAK mible_status_t mible_iic_init(const iic_config_t * p_config,
        mible_handler_t handler)
{
    return MI_SUCCESS;
}

/**
 * @brief   Function for uninitializing the IIC driver instance.
 * 
 *              
 * */
__WEAK void mible_iic_uninit(void)
{

}

/**
 * @brief   Function for sending data to a IIC slave.
 * @param   [in] addr:   Address of a specific slave device (only 7 LSB).
 *          [in] p_out:  Pointer to tx data
 *          [in] len:    Data length
 *          [in] no_stop: If set, the stop condition is not generated on the bus
 *          after the transfer has completed successfully (allowing for a repeated start in the next transfer).
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_out is not vaild address.
 * @note    This function should be implemented in non-blocking mode.
 *          When tx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument. 
 * */
__WEAK mible_status_t mible_iic_tx(uint8_t addr, uint8_t * p_out, uint16_t len,
bool no_stop)
{
    return MI_SUCCESS;
}

/**
 * @brief   Function for receiving data from a IIC slave.
 * @param   [in] addr:   Address of a specific slave device (only 7 LSB).
 *          [out] p_in:  Pointer to rx data
 *          [in] len:    Data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_in is not vaild address.
 * @note    This function should be implemented in non-blocking mode.
 *          When rx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument. 
 * */
__WEAK mible_status_t mible_iic_rx(uint8_t addr, uint8_t * p_in, uint16_t len)
{
    return MI_SUCCESS;
}

/**
 * @brief   Function for checking IIC SCL pin.
 * @param   [in] port:   SCL port
 *          [in] pin :   SCL pin
 * @return  1: High (Idle)
 *          0: Low (Busy)
 * */
__WEAK int mible_iic_scl_pin_read(uint8_t port, uint8_t pin)
{
    return 0;
}

void mijia_gatt_db_init(void)
{
	memset(gMiJia_TimerPool, 0, sizeof(Mijia_Timer_db) * MIBLE_TIMER_MAX_NUM);
	
	if(WICED_BT_SUCCESS== wiced_init_timer( &mible_task_post_timer, &mible_task_post_handler, 0, WICED_MILLI_SECONDS_TIMER ))
	{
		MI_LOG_INFO("BT wiced_init_timer SUCESS\r\n");
	}	
    queue_init(&task_queue, task_buf, sizeof(task_buf)/sizeof(task_buf[0]), sizeof(task_buf[0]));
	
    // token
    gMibleGatt_db[0].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[0].handle_uuid=UUID_CHARACTERISTIC_TOKEN;
    gMibleGatt_db[0].conn_handle=MI_IDX_TOKEN_CHAR;
    gMibleGatt_db[0].handle_val=MI_IDX_TOKEN_VAL;
    gMibleGatt_db[0].handle_max_len=UUID_TOKEN_SIZE;//MIBLE_TOKEN_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[0].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[0].handle_max_len);
    memset(gMibleGatt_db[0].pUcData,'\0',gMibleGatt_db[0].handle_max_len);
    //PRODUCT ID
    gMibleGatt_db[1].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[1].handle_uuid= UUID_CHARACTERISTIC_PRODUCT_ID;
    gMibleGatt_db[1].conn_handle=MI_IDX_PRODUCT_ID_CHAR;
    gMibleGatt_db[1].handle_val=MI_IDX_PRODUCT_ID_VAL;
    gMibleGatt_db[1].handle_max_len=UUID_PRODUCT_ID_SIZE;//MIBLE_PRODUCT_ID_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[1].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[1].handle_max_len);
    memset(gMibleGatt_db[1].pUcData,'\0',gMibleGatt_db[1].handle_max_len);

    //version
    gMibleGatt_db[2].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[2].handle_uuid= UUID_CHARACTERISTIC_VERSION;
    gMibleGatt_db[2].conn_handle=MI_IDX_VERSION_ID_CHAR;
    gMibleGatt_db[2].handle_val=MI_IDX_VERSION_ID_VAL;
    gMibleGatt_db[2].handle_max_len=UUID_VERSION_SIZE;//MIBLE_VERSION_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[2].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[2].handle_max_len);
    memset(gMibleGatt_db[2].pUcData,'\0',gMibleGatt_db[2].handle_max_len);

	//WiFi config
	gMibleGatt_db[3].service_handle=SQUIRREL_IDX_SVC;
	gMibleGatt_db[3].handle_uuid= UUID_CHARACTERISTIC_WIFICFG;
	gMibleGatt_db[3].conn_handle=MI_IDX_WIFICFG_CHAR;
	gMibleGatt_db[3].handle_val=MI_IDX_WIFICFG_VAL;	 
	gMibleGatt_db[3].handle_max_len=UUID_WIFI_CONFIG_SIZE;//MIBLE_VERSION_HANDLE_TRANSPORT_MAX_NUM;
	gMibleGatt_db[3].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[3].handle_max_len);
	memset(gMibleGatt_db[3].pUcData,'\0',gMibleGatt_db[3].handle_max_len);

    //AUTHENTICATION
    gMibleGatt_db[4].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[4].handle_uuid= UUID_CHARACTERISTIC_AUTHENTICATION;
    gMibleGatt_db[4].conn_handle=MI_IDX_AUTHENTICATION_CHAR;
    gMibleGatt_db[4].handle_val=MI_IDX_AUTHENTICATION_VAL;
    gMibleGatt_db[4].handle_max_len=UUID_AUTHENCATION_SIZE;//MIBLE_AUTHENTICATION_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[4].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[4].handle_max_len);
    memset(gMibleGatt_db[4].pUcData,'\0',gMibleGatt_db[4].handle_max_len);

    //device id
    gMibleGatt_db[5].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[5].handle_uuid=UUID_CHARACTERISTIC_DEVICE_ID;
    gMibleGatt_db[5].conn_handle=MI_IDX_DEVICE_ID_CHAR;
    gMibleGatt_db[5].handle_val=MI_IDX_DEVICE_ID_VAL;
    gMibleGatt_db[5].handle_max_len=UUID_DEVICE_ID_SIZE;//MIBLE_DEVICE_ID_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[5].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[5].handle_max_len);
    memset(gMibleGatt_db[5].pUcData,'\0',gMibleGatt_db[5].handle_max_len);

  
	//beacon key
    gMibleGatt_db[6].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[6].handle_uuid=UUID_CHARACTERISTIC_BEACON_KEY;
    gMibleGatt_db[6].conn_handle=MI_IDX_BEACON_KEY_CHAR;
    gMibleGatt_db[6].handle_val=MI_IDX_BEACON_KEY_VAL;
    gMibleGatt_db[6].handle_max_len=UUID_BEACON_KEY_SIZE;//MIBLE_BEACON_KEY_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[6].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[6].handle_max_len);
    memset(gMibleGatt_db[6].pUcData,'\0',gMibleGatt_db[6].handle_max_len);

    //MI_LOG_INFO("Gatts_Handle_Value_Init");
}

int mijia_gatt_write(uint16_t conn_id, void *ucReq )
{
    uint8_t i,j=0;
    mible_gatts_evt_param_t gattsParam={0};
    wiced_bt_gatt_write_t *p_req=(wiced_bt_gatt_write_t *)ucReq;
   
    MI_LOG_INFO("mijia_gatt_write len=%d, data=", p_req->val_len);
	MI_LOG_HEXDUMP(p_req->p_val,  p_req->val_len);

    if (p_req->val_len == 0)
    {
        MI_LOG_INFO("mijia_gatt_write MI_ERR_INVALID_LENGTH1\r\n");
        return MI_ERR_INVALID_LENGTH;
    }

    for(i=0;i<MIBLE_GATT_HANDLE_MAX_NUM;i++)
    {
        if(p_req->handle == gMibleGatt_db[i].handle_val)
        {
			//MI_LOG_INFO("mijia_gatt_write: char handle");
            break;
        }
    }
	
    gMibleGatt_db[i].offset=p_req->offset;
    gMibleGatt_db[i].ucDataLen=p_req->val_len;
	gMibleGatt_db[i].handle_max_len=p_req->val_len;
	memcpy(gMibleGatt_db[i].pUcData, p_req->p_val, p_req->val_len);

	MI_LOG_INFO("\r\nWritten gatt(XiaoMi):char uuid handle=0x%2x; length=%d; data=", p_req->handle, p_req->val_len);	
	MI_LOG_HEXDUMP(gMibleGatt_db[i].pUcData, gMibleGatt_db[i].ucDataLen);

	gattsParam.conn_handle=conn_id;
	gattsParam.write.value_handle=p_req->handle;
	gattsParam.write.data=p_req->p_val;
	gattsParam.write.len=p_req->val_len;
	gattsParam.write.offset=p_req->offset;

	mible_gatts_event_callback(MIBLE_GATTS_EVT_WRITE,&gattsParam);
	//mible_std_gatts_event_handler(MIBLE_GATTS_EVT_WRITE,&gattsParam);
	
	return MI_SUCCESS;
}

int mijia_gatt_read(uint16_t handle,uint8_t *readData,uint16_t *len,uint8_t offset)
{
    uint8_t i=0;
	
    for(i=0;i<MIBLE_GATT_HANDLE_MAX_NUM;i++)
    {
        if(handle==gMibleGatt_db[i].handle_val)
        {
            //MI_LOG_INFO("mijia_gatt_read: reading mijia char handle");
            break;
        }
    }
	 
    if((offset > gMibleGatt_db[i].handle_max_len))
    {
        MI_LOG_INFO("mijia_gatt_read: MI_ERR_INVALID_LENGTH2\r\n");
        return MI_ERR_INVALID_LENGTH;
    }
    if(*len > gMibleGatt_db[i].handle_max_len)
    {
        *len = gMibleGatt_db[i].handle_max_len;
    }
    if((offset>0) && (offset <gMibleGatt_db[i].handle_max_len))
    {
        *len = (gMibleGatt_db[i].handle_max_len-offset);
    }
    memcpy(readData,gMibleGatt_db[i].pUcData+offset,*len);

	MI_LOG_INFO("\r\nRead gatt(XiaoMi):char uuid hand=0x%2x, offset=%d, length=%d, data=",handle, offset, *len);
	MI_LOG_HEXDUMP(readData,*len);

}

int mijia_gap_setup(uint8_t evt,void *ucStatus)
{
    mible_gap_evt_param_t gapParam;
    wiced_bt_gatt_connection_status_t *p_status=(wiced_bt_gatt_connection_status_t *)ucStatus;
    if(evt==MIBLE_GAP_EVT_CONNECTED)
    {
        gapParam.conn_handle=p_status->conn_id;
        memcpy(gapParam.connect.peer_addr,p_status->bd_addr,6);
        gapParam.connect.role=p_status->link_role;
        gapParam.connect.type=p_status->addr_type;
		
        mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED,&gapParam);
		//mible_std_gap_event_handler(MIBLE_GAP_EVT_CONNECTED,&gapParam);
   
    }
    else if(evt==MIBLE_GAP_EVT_DISCONNET)
    {
        gapParam.conn_handle=p_status->conn_id;
        if((p_status->reason>1) && (p_status->reason<5))
        {
            gapParam.disconnect.reason=(p_status->reason-1);
        }
		
        mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNET,&gapParam);
		//mible_std_gap_event_handler(MIBLE_GAP_EVT_CONNECTED,&gapParam);
    }
    else
    {
        ;
    }
}

void advertising_init(void)
{
    MI_LOG_INFO("\r\nadvertising init...\r\n");
     mibeacon_frame_ctrl_t frame_ctrl = {
    .time_protocol = 0,
    .is_encrypt = 0,
    .mac_include = 1,
    .cap_include = 1,
    .obj_include = 0,
    .bond_confirm = 0,
    .version = 0x03,
    };
    mibeacon_capability_t cap = {.connectable = 1,
                                .encryptable = 1,
                                .bondAbility = 1};
    mible_addr_t dev_mac;
    mible_gap_address_get(dev_mac);
    
    mibeacon_config_t mibeacon_cfg = {
    .frame_ctrl = frame_ctrl,
    .pid =dev_info.pid,
    .p_mac = (mible_addr_t*)dev_mac, 
    .p_capability = &cap,
    .p_obj = NULL,
    };
    
    uint8_t service_data[31];
	uint8_t service_data_len=0;
	
	if(MI_SUCCESS != mible_service_data_set(&mibeacon_cfg, service_data, &service_data_len)){
		MI_LOG_ERROR("\r\n mible_service_data_set: failed! \r\n");
		return;
	}

	uint8_t adv_data[23]={0};
	uint8_t adv_len=0;
	
	//add flags
	adv_data[0] = 0x02;
	adv_data[1] = 0x01;
	adv_data[2] = 0x06;
	
	memcpy(adv_data+3, service_data, service_data_len);
	adv_len = service_data_len + 3;
	
	mible_gap_adv_data_set(adv_data,adv_len,NULL,0);
	
	//MI_LOG_INFO("\r\n adv mi service data:");
	//MI_LOG_HEXDUMP(adv_data, adv_len);
	//MI_LOG_PRINTF("\r\n");
	return;
}

void advertising_start(void){
     mible_gap_adv_param_t adv_param =(mible_gap_adv_param_t){
    .adv_type = MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
    .adv_interval_min = 0x00a0,//MSEC_TO_UNITS(100, UNIT_0_625_MS),
    .adv_interval_max = 0x00b0,//MSEC_TO_UNITS(200, UNIT_0_625_MS),
    .ch_mask = {0},
    };
    if(MI_SUCCESS != mible_gap_adv_start(&adv_param)){
        MI_LOG_ERROR("mible_gap_adv_start failed. \r\n");
        return;
    }
   
}

void mible_service_init_cmp(void)
{
    MI_LOG_INFO("mible_service_init_cmp\r\n");
}

void mible_connected(void)
{
    MI_LOG_INFO("mible_connected \r\n");
}

void mible_disconnected(void)
{
    MI_LOG_INFO("mible_disconnected \r\n");
    advertising_start();
}

void mible_bonding_evt_callback(mible_bonding_state state)
{
    if(state == BONDING_FAIL){
        MI_LOG_INFO("BONDING_FAIL\r\n");
        mible_gap_disconnect(mible_server_connection_handle);
    }else if(state == BONDING_SUCC){
        MI_LOG_INFO("BONDING_SUCC\r\n");
    }else if(state == LOGIN_FAIL){
        MI_LOG_INFO("LOGIN_FAIL\r\n");
        mible_gap_disconnect(mible_server_connection_handle);
    }else{
        MI_LOG_INFO("LOGIN_SUCC\r\n");
    }
}

