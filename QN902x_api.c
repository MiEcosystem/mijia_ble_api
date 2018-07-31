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
/*
 * Add your own include file
 *
 * */  
#include "app_env.h"
#include "nvds.h"
#include "gap.h"
#include "aes.h"
#include "rng.h"


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
    nvds_tag_len_t bd_addr_length = NVDS_LEN_BD_ADDRESS;
    if (NVDS_OK != nvds_get(NVDS_TAG_BD_ADDRESS, &bd_addr_length, mac))
        return MI_ERR_INTERNAL;
    else
        return MI_SUCCESS;
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
    return MI_SUCCESS;
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
static uint8_t adv_data_len;
static uint8_t rsp_data_len;
mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
    uint16_t mode;
    switch(p_param->adv_type)
    {
        case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:
            mode = GAP_GEN_DISCOVERABLE | GAP_UND_CONNECTABLE;
            break;
        case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:
            mode = GAP_BROADCASTER;
            break;
        case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:
            mode = GAP_NON_CONNECTABLE;
            break;
        
    }
    app_gap_adv_start_req(mode, 
                        app_env.adv_data, adv_data_len, 
                        app_env.scanrsp_data, rsp_data_len,
                        p_param->adv_interval_min, p_param->adv_interval_max);
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
    if((dlen > ADV_DATA_LEN) || (srdlen > SCAN_RSP_DATA_LEN))
        return MI_ERR_INVALID_PARAM;
    
    if(p_data != NULL)
    {
        memcpy(app_env.adv_data, p_data, dlen);
        adv_data_len = dlen;
    }
    if(p_sr_data != NULL)
    {
        memcpy(app_env.scanrsp_data, p_sr_data, srdlen);
        rsp_data_len = srdlen;
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
    app_gap_adv_stop_req();
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
    return MI_SUCCESS;
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
    app_gap_discon_req(conn_handle);
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
    struct gap_conn_param_update conn_par;
    /// Connection interval minimum
    conn_par.intv_min = conn_params.min_conn_interval;
    /// Connection interval maximum
    conn_par.intv_max = conn_params.max_conn_interval;
    /// Latency
    conn_par.latency = conn_params.slave_latency;
    /// Supervision timeout, Time = N * 10 msec
    conn_par.time_out = conn_params.conn_sup_timeout;
    
    app_gap_param_update_req(conn_handle, &conn_par);
    
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
    volatile uint8_t srvNum = 0;
    volatile  uint8_t charNum = 0;
    volatile  mible_gatts_srv_db_t *svr_db;
    volatile  mible_gatts_char_db_t *char_db;
    
    volatile  struct atts_char_desc* p_atts_char_desc;
    volatile  uint16_t uuid;
    
    for(srvNum = 0; srvNum < p_server_db->srv_num; srvNum++)
    {
        svr_db = &(p_server_db->p_srv_db[srvNum]);
        uuid = *(uint16_t*)mis_att_db[0].value;
        if(svr_db->srv_uuid.uuid16 == uuid)
        {
            svr_db->srv_handle = mis_env.mis_shdl;
            
            for(charNum = 0; charNum < svr_db->char_num; charNum++)
            {
                char_db = &(svr_db->p_char_db[charNum]);
                for(uint8_t i = 0; i < MIS_IDX_NB; i++)
                {
                    if(mis_att_db[i].uuid == ATT_DECL_CHARACTERISTIC)
                    {
                        p_atts_char_desc = (struct atts_char_desc*)(mis_att_db[i].value);
                        uuid = p_atts_char_desc->attr_type[0] + (p_atts_char_desc->attr_type[1] << 8);
                        if(char_db->char_uuid.uuid16 == uuid)
                        {
                            char_db->char_value_handle = i + 1 + svr_db->srv_handle;
                            //if((p_atts_char_desc->prop & ATT_CHAR_PROP_RD) == ATT_CHAR_PROP_RD)
                            {
                                attsdb_att_set_value(char_db->char_value_handle, char_db->char_value_len, char_db->p_value);
                                //QPRINTF("att handle:%x, len: %d, value:%s\r\n", char_db->char_value_handle,char_db->char_value_len, char_db->p_value);
                            }
                        }
                    }
                }
            }
        }
    }
    
    static mible_arch_evt_param_t param;
    memset(&param, 0, sizeof(param));
    param.srv_init_cmp.status = MI_SUCCESS;
    param.srv_init_cmp.p_gatts_db = p_server_db;
    mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
    
    return MI_SUCCESS;
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
    attsdb_att_set_value(value_handle, len, p_value);
    return MI_SUCCESS;
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
    atts_size_t size;
    uint8_t* pp_value;
    if(ATT_ERR_NO_ERROR == attsdb_att_get_value(value_handle, &size, &pp_value))
    {
        *p_len = size;
        memcpy(p_value, pp_value, size);
    }
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
    if(1 == type)
    {
        //Update value in DB
        attsdb_att_set_value(char_value_handle, len, (void *)p_value);
        
        // The notification can be sent, send the notification
        struct gatt_notify_req *ntf = KE_MSG_ALLOC(GATT_NOTIFY_REQ,
                                                   TASK_GATT, KE_BUILD_ID(TASK_MIS, conn_handle),
                                                   gatt_notify_req);
        ntf->conhdl  = conn_handle;
        ntf->charhdl = char_value_handle;

        ke_msg_send(ntf);
    }
    else if(2 == type)
    {
        //Update value in DB
        attsdb_att_set_value(char_value_handle, len, (void *)p_value);
        
        struct gatt_indicate_req *msg = KE_MSG_ALLOC(GATT_INDICATE_REQ, TASK_GATT, KE_BUILD_ID(TASK_MIS, conn_handle),
                                                        gatt_indicate_req);
        //Connection handle
        msg->conhdl = conn_handle;
        //Characteristic handle
        msg->charhdl = char_value_handle;

        ke_msg_send(msg);
    }
    else
    {
        return MI_ERR_INVALID_PARAM;
    }

    // Go to Busy state
    ke_state_set(KE_BUILD_ID(TASK_MIS, conn_handle), MIS_BUSY);
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
mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle,
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
mible_status_t mible_gattc_primary_service_discover_by_uuid(
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
mible_status_t mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
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
mible_status_t mible_gattc_clt_cfg_descriptor_discover(
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
mible_status_t mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
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
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle,
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
mible_status_t mible_gattc_write_cmd(uint16_t conn_handle,
        uint16_t att_handle, uint8_t* p_value, uint8_t len)
{
    return MI_SUCCESS;
}

/**
 *        SOFT TIMER APIs
 */
typedef struct {
    uint8_t id;
    mible_timer_handler cb;
    mible_timer_mode type;
    uint32_t timeout;
    void * par;
}mible_timer_t;

#define MIBLE_TIMER_MAX  5
mible_timer_t mible_timer[MIBLE_TIMER_MAX];

uint8_t find_exist_timer(uint8_t id)
{
    uint8_t index = 0;
    for(index=0; index < MIBLE_TIMER_MAX; index++)
    {
        if(mible_timer[index].id == id)
            break;
    }
    return index;
}

uint8_t find_empty_timer()
{
    uint8_t index = 0;
    for(index=0; index < MIBLE_TIMER_MAX; index++)
    {
        if(mible_timer[index].cb == NULL)
            break;
    }
    return index;
}

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
    uint8_t index;
    uint8_t p_id;

    index = find_empty_timer();
    if(index < MIBLE_TIMER_MAX)
    {
        p_id = index | 0x80;
        mible_timer[index].id = p_id;
        mible_timer[index].cb = timeout_handler;
        mible_timer[index].type = mode ;
        
        *p_timer_id = (void*)p_id;
    }
    else
    {
        return MI_ERR_INVALID_PARAM;
    }
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
    uint8_t index;
    
    index = find_exist_timer((uint8_t)timer_id);
    if(index < MIBLE_TIMER_MAX)
    {
        ke_timer_clear(APP_MIBLE_TIMER0 + index, TASK_APP);
        mible_timer[index].id = 0;
        mible_timer[index].cb = NULL;
    }
    else
    {
        return MI_ERR_INVALID_PARAM;
    }
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
    uint8_t index;
    
    index = find_exist_timer((uint8_t)timer_id);
    if(index < MIBLE_TIMER_MAX)
    {
        ke_timer_set(APP_MIBLE_TIMER0 + index, TASK_APP, timeout_value);
        mible_timer[index].timeout = timeout_value;
        mible_timer[index].par = p_context;
    }
    else
    {
        return MI_ERR_INVALID_PARAM;
    }
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
    uint8_t index;
    
    index = find_exist_timer((uint8_t)timer_id);
    if(index < MIBLE_TIMER_MAX)
    {
        ke_timer_clear(APP_MIBLE_TIMER0 + index, TASK_APP);
    }
    else
    {
        return MI_ERR_INVALID_PARAM;
    }
    return MI_SUCCESS;
}
/**
 ****************************************************************************************
 * @brief Handles mible timer.
 *
 * @param[in] msgid     Id of the message received
 * @param[in] param     None
 * @param[in] dest_id   TASK_APP
 * @param[in] src_id    TASK_APP
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_mible_timer_handler(ke_msg_id_t const msgid, void const *param,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t index;
    
    if(msgid > APP_MIBLE_TIMER4)
        ASSERT_ERR(0);

    index = msgid - APP_MIBLE_TIMER0;
    
    if(mible_timer[index].type == MIBLE_TIMER_REPEATED)
        ke_timer_set(APP_MIBLE_TIMER0 + index, TASK_APP, mible_timer[index].timeout);
    else
        ke_timer_clear(APP_MIBLE_TIMER0 + index, TASK_APP);

    mible_timer[index].cb(mible_timer[index].par);

    return (KE_MSG_CONSUMED);
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
    uint8_t err = MI_SUCCESS;
    return (mible_status_t)err;
}

/**
 * @brief   Delete a record in flash
 * @param   [in] record_id: identify a record in flash  
 * @return  MI_SUCCESS              Delete successfully. 
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
    uint8_t err = 0;
    
    record_id  += 100;
#if (QN_NVDS_WRITE)
    err = nvds_del(record_id);
#endif
    return (mible_status_t)err;
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
    uint16_t nvds_len   = len;
    uint8_t err;
    
    record_id  += 100;

    err = nvds_get(record_id, &nvds_len, p_data);
    return (mible_status_t)err;
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
    uint8_t err;
    
    record_id  += 100;
#if (QN_NVDS_WRITE)
    err = nvds_put(record_id, len, p_data);
#endif
    mible_arch_evt_param_t par;
    par.record.id = record_id;
    par.record.status = (mible_status_t)err;

    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &par);

    return (mible_status_t)err;
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
    uint32_t rngVal = 0;
    for(int i = len/4; i > 0; i--)
    {
        rngVal = rng_get();
        *(uint32_t*)p_buf = rngVal;
        p_buf += 4;
    }
    rngVal = rng_get();
    for(int i = len%4; i > 0; i--)
    {
        *p_buf++ = (rngVal >> 8*i) & 0xFF;
    }
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
    uint8_t plain[16] = {0};
//    uint8_t cipher[16];
    aes_context ctx;
    
    if( plaintext == NULL || key == NULL)
        return MI_ERR_INVALID_ADDR;
    else if (plen > 16)
        return MI_ERR_INVALID_LENGTH;
    
    memcpy(plain, (void*)plaintext, plen);
    aes_set_key(&ctx, key, 128);
    aes_encrypt_block(&ctx, ciphertext, plaintext);
    
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

/// Message of mible handler request
struct app_mible_tasks
{
    void * arg;
    mible_handler_t handler;
};

mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
    struct app_mible_tasks *req = ke_msg_alloc(APP_MIBLE_TASKS_IND,
                                                TASK_APP,
                                                TASK_NONE,
                                                sizeof(struct app_mible_tasks));
    req->handler = handler;
    req->arg = arg;
    ke_msg_send(req);
    
    return MI_SUCCESS;
}

/**
 * @brief   Function for executing all enqueued tasks.
 *
 * @note    This function must be called from within the main loop. It will 
 * execute all events scheduled since the last time it was called.
 * */
void mible_tasks_exec(void)
{

}

/**
 ****************************************************************************************
 * @brief Handles mible tasks indication.
 *
 * @param[in] msgid      APP_MIBLE_TASKS_IND
 * @param[in] param      Pointer to app_mible_tasks
 * @param[in] dest_id    TASK_APP
 * @param[in] src_id     TASK_APP
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_mible_tasks_handler(ke_msg_id_t const msgid, struct app_mible_tasks const *ind,
                               ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    ind->handler(ind->arg);
    
    return (KE_MSG_CONSUMED);
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
mible_status_t mible_iic_init(const iic_config_t * p_config,
        mible_handler_t handler)
{
    return MI_SUCCESS;
}

/**
 * @brief   Function for uninitializing the IIC driver instance.
 * 
 *              
 * */
void mible_iic_uninit(void)
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
mible_status_t mible_iic_tx(uint8_t addr, uint8_t * p_out, uint16_t len,
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
mible_status_t mible_iic_rx(uint8_t addr, uint8_t * p_in, uint16_t len)
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
int mible_iic_scl_pin_read(uint8_t port, uint8_t pin)
{
    return 0;
}

