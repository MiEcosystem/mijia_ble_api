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
#include "mible_log.h"

#include "nrf_soc.h"
#include "nrf_queue.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "ble_types.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_gatts.h"

#include "app_timer.h"
#include "app_util_platform.h"
#include "nrf_drv_twi_patched.h"

#include "mi_psm.h"

#define TIMER_MAX_NUM                   4
#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */


static mible_status_t err_code_convert(uint32_t errno)
{
    mible_status_t stat;
    switch (errno) {
    case NRF_SUCCESS:
        stat = MI_SUCCESS;
        break;
    case NRF_ERROR_INTERNAL:
        stat = MI_ERR_INTERNAL;
        break;
    case NRF_ERROR_NOT_FOUND:
        stat = MI_ERR_NOT_FOUND;
        break;
    case NRF_ERROR_NO_MEM:
        stat = MI_ERR_NO_MEM;
        break;
    case NRF_ERROR_INVALID_ADDR:
        stat = MI_ERR_INVALID_ADDR;
        break;
    case NRF_ERROR_INVALID_PARAM:
        stat = MI_ERR_INVALID_PARAM;
        break;
    case NRF_ERROR_INVALID_STATE:
        stat = MI_ERR_INVALID_STATE;
        break;
    case NRF_ERROR_INVALID_LENGTH:
        stat = MI_ERR_INVALID_LENGTH;
        break;
    case NRF_ERROR_DATA_SIZE:
        stat = MI_ERR_DATA_SIZE;
        break;
    case NRF_ERROR_BUSY:
        stat = MI_ERR_BUSY;
        break;
    case NRF_ERROR_TIMEOUT:
        stat = MI_ERR_TIMEOUT;
        break;

    }

    return stat;
}

/*
 * @brief   Get BLE mac address.
 * @param   [out] mac: pointer to data
 * @return  MI_SUCCESS          The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note:   You should copy gap mac to mac[6]  
 * */
mible_status_t mible_gap_address_get(mible_addr_t mac)
{
    uint32_t errno;
    ble_gap_addr_t gap_addr;
    #if (NRF_SD_BLE_API_VERSION == 3)
        errno = sd_ble_gap_addr_get(&gap_addr);
    #else
        errno = sd_ble_gap_address_get(&gap_addr);
    #endif
    memcpy(mac, gap_addr.addr, 6);
    return err_code_convert(errno);
}

/* GAP related function.  You should complement these functions. */

/*
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
    uint32_t errno;
    ble_gap_scan_params_t  sdk_param = {0};
    sdk_param.active = scan_type == MIBLE_SCAN_TYPE_ACTIVE ? 1 : 0;
    sdk_param.interval = scan_param.scan_interval;
    sdk_param.window   = scan_param.scan_window;
    sdk_param.timeout  = scan_param.timeout;
    errno = sd_ble_gap_scan_start(&sdk_param);
    return err_code_convert(errno);
}

/*
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void)
{
    uint32_t errno;
    errno = sd_ble_gap_scan_stop() == MI_SUCCESS ?  MI_SUCCESS : MI_ERR_INVALID_STATE;
    return err_code_convert(errno); 
}

/*
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
mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_adv_param)
{
    uint32_t errno;
    if (p_adv_param == NULL )
        return MI_ERR_INVALID_PARAM;

    sd_ble_gap_adv_stop();
    
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
    
    return err_code_convert(errno);
}

/*
 * @brief   Config advertising data
 * @param   [in] p_data : Raw data to be placed in advertising packet. If NULL, no changes are made to the current advertising packet.
 * @param   [in] dlen   : Data length for p_data. Max size: 31 octets. Should be 0 if p_data is NULL, can be 0 if p_data is not NULL.
 * @param   [in] p_sr_data : Raw data to be placed in scan response packet. If NULL, no changes are made to the current scan response packet data.
 * @param   [in] srdlen : Data length for p_sr_data. Max size: BLE_GAP_ADV_MAX_SIZE octets. Should be 0 if p_sr_data is NULL, can be 0 if p_data is not NULL.
 * @return  MI_SUCCESS             Successfully set advertising data.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * */   
mible_status_t mible_gap_adv_data_set(uint8_t const * p_data, uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
    uint32_t errno;
    errno = sd_ble_gap_adv_data_set(p_data, dlen, p_sr_data, srdlen);
    MI_ERR_CHECK(errno);
    return err_code_convert(errno);
}


/*
 * @brief   Stop advertising
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void)
{
    uint32_t errno;
    errno = sd_ble_gap_adv_stop();
    errno = errno == NRF_SUCCESS ? MI_SUCCESS : MI_ERR_INVALID_STATE;
    return err_code_convert(errno);
}

/*
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
    uint32_t errno;
    ble_gap_addr_t mac;
    ble_gap_scan_params_t sdk_scan_param = {0};
    ble_gap_conn_params_t sdk_conn_param = {0};
    switch (conn_param.type) {
        case MIBLE_ADDRESS_TYPE_PUBLIC:
            mac.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
            break;
        case MIBLE_ADDRESS_TYPE_RANDOM:
            mac.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
            break;
    }
    memcpy(mac.addr, conn_param.peer_addr, 6);
    
    sdk_scan_param.active   = 1;
    sdk_scan_param.interval = scan_param.scan_interval;
    sdk_scan_param.window   = scan_param.scan_window;
    sdk_scan_param.timeout  = scan_param.timeout;

    sdk_conn_param.min_conn_interval = conn_param.conn_param.min_conn_interval;
    sdk_conn_param.max_conn_interval = conn_param.conn_param.max_conn_interval;
    sdk_conn_param.slave_latency     = conn_param.conn_param.slave_latency;
    sdk_conn_param.conn_sup_timeout  = conn_param.conn_param.conn_sup_timeout;
    errno = sd_ble_gap_connect(&mac, &sdk_scan_param, &sdk_conn_param);
    return err_code_convert(errno);
}

/*
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
    uint32_t errno;
    errno = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    errno = errno == NRF_SUCCESS ? MI_SUCCESS : MI_ERR_INVALID_STATE;
    return err_code_convert(errno);
}

/*
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
    uint32_t errno;
    errno = sd_ble_gap_conn_param_update(conn_handle, (ble_gap_conn_params_t*)&conn_params);
    errno = errno == NRF_SUCCESS ? MI_SUCCESS : MI_ERR_INVALID_STATE;
    return err_code_convert(errno);
}

/* GATTS related function  */

static ble_uuid_t convert_uuid(mible_uuid_t *p_uuid)
{
    uint32_t errno;
    ble_uuid_t uuid16 = {0};
    if (p_uuid->type == 0) {
        uuid16.type = BLE_UUID_TYPE_BLE;
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

    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
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
    return err_code_convert(errno);
}

/*
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
    uint32_t errno;
    ble_uuid_t uuid16 = {0};
    
    if (p_server_db == NULL)
        return MI_ERR_INVALID_ADDR;

    for (uint8_t idx = 0, max = p_server_db->srv_num; idx < max; idx++) {
        mible_gatts_srv_db_t *p_service = p_server_db->p_srv_db + idx;

        // add vendor specific uuid
        uuid16 = convert_uuid(&p_service->srv_uuid);
        
        // add service
        errno = sd_ble_gatts_service_add(
            p_service->srv_type == MIBLE_PRIMARY_SERVICE ? BLE_GATTS_SRVC_TYPE_PRIMARY : BLE_GATTS_SRVC_TYPE_SECONDARY,
            &uuid16, &p_service->srv_handle);
        MI_ERR_CHECK(errno);

        // add charateristic
        for (uint8_t idx = 0, max = p_service->char_num; idx < max; idx++) {
            mible_gatts_char_db_t *p_char = p_service->p_char_db + idx;
            ble_gatt_char_props_t props = {0};
            memcpy((uint8_t*)&props, &p_char->char_property, 1);
            
            uuid16 = convert_uuid(&p_char->char_uuid);
            errno = char_add(p_service->srv_handle, &uuid16, p_char->p_value, p_char->char_value_len,
                        props, &p_char->char_value_handle,
                        p_char->is_variable_len, p_char->rd_author, p_char->wr_author);
            MI_ERR_CHECK(errno);
        }
    }
    mible_arch_evt_param_t param;
    memset(&param, 0, sizeof(param));
    param.srv_init_cmp.status = MI_SUCCESS;
    param.srv_init_cmp.p_gatts_db = p_server_db;
    mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
    return MI_SUCCESS;
}

/*
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
    
    return err_code_convert(errno);
}

/*
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
mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t char_handle,
    uint8_t* p_value, uint8_t *p_len)
{
    uint32_t errno;
    ble_gatts_value_t value = {0};
    errno = sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, char_handle, &value);
    MI_ERR_CHECK(errno);
    memcpy(p_value, value.p_value, value.len);
    *p_len = value.len;

    return err_code_convert(errno);
}

/*
 * @brief   Respond to a Read/Write user authorization request.
 * @param   [in] conn_handle: conn handle
 *          [in] status:  1: permit access ; 0: reject access
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
mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle, uint8_t status,
    uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
    uint8_t len, uint8_t type)
{
    ble_gatts_rw_authorize_reply_params_t reply = {0};
    
    if (status == 1) {
        switch (type) {
        case 1:
            reply = (ble_gatts_rw_authorize_reply_params_t) {
                .type = BLE_GATTS_AUTHORIZE_TYPE_READ,
                .params.read.gatt_status = BLE_GATT_STATUS_SUCCESS,
                .params.read.offset      = offset,
                .params.read.p_data      = p_value,
                .params.read.len         = len,
            };
        break;

        case 2:
            reply = (ble_gatts_rw_authorize_reply_params_t) {
                .type = BLE_GATTS_AUTHORIZE_TYPE_WRITE,
                .params.write.gatt_status = BLE_GATT_STATUS_SUCCESS,
                .params.write.update      = 1,
                .params.write.offset      = offset,
                .params.write.p_data      = p_value,
                .params.write.len         = len
            };
        break;
        }
    } else if (status == 0) {
        switch (type) {
        case 1:
            reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
            reply.params.read.gatt_status = BLE_GATT_STATUS_ATTERR_READ_NOT_PERMITTED;
        break;

        case 2:
            reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
            reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
        break;
        }
    }

    uint32_t errno = sd_ble_gatts_rw_authorize_reply(conn_handle, &reply);
    MI_ERR_CHECK(errno);

    return err_code_convert(errno);
}

// this function set char value and notify/indicate it to client
/*
 * @brief   Set characteristic value and notify it to client.
 * @param   [in] conn_handle: conn handle
 *          [in] srv_handle: service handle
 *          [in] char_value_handle: characteristic handle
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
        .type   = type == 1 ? BLE_GATT_HVX_NOTIFICATION : BLE_GATT_HVX_INDICATION,
        .offset = offset,
        .p_data = p_value,
        .p_len  = &length
    };
    errno = sd_ble_gatts_hvx(conn_handle, &hvx);
    return err_code_convert(errno);
}



/* GATTC related function */

/*
 * @brief   Discover primary service by service UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for primary sevice
 *discovery procedure
 *          [in] uuid_type: 16-bit or 128-bit
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
mible_status_t mible_gattc_primary_service_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range, mible_uuid_t* p_srv_uuid)
{
    uint32_t errno;
    ble_uuid_t srv_uuid;
    srv_uuid = convert_uuid(p_srv_uuid);
    errno = sd_ble_gattc_primary_services_discover(conn_handle, handle_range.begin_handle, &srv_uuid);
    return err_code_convert(errno);
}

/*
 * @brief   Discover characteristic by characteristic UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for characteristic discovery
 * procedure
 *          [in] uuid_type: 16-bit or 128-bit
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
    mible_handle_range_t handle_range,
    mible_uuid_t* p_char_uuid)
{
    
    return MI_SUCCESS;
}

/*
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
mible_status_t mible_gattc_clt_cfg_descriptor_discover(uint16_t conn_handle,
    mible_handle_range_t handle_range)
{
    return MI_SUCCESS;
}

/*
 * @brief   Read characteristic value by UUID
 * @param   [in] conn_handle: connnection handle
 *          [in] handle_range: search range
 *          [in] uuid_type: 16-bit or 128-bit
 *          [in] char_uuid: characteristic uuid
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
    mible_uuid_t *p_char_uuid)
{
    return MI_SUCCESS;
}

/*
 * @brief   Write characteristic value by handle
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
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle, uint16_t handle,
    uint8_t* p_value, uint8_t len)
{
    uint32_t errno;
    ble_gattc_write_params_t params = {0};
    params.write_op = BLE_GATT_OP_WRITE_REQ;
    params.handle   = handle;
    params.p_value  = p_value;
    params.len      = len;
    
    errno = sd_ble_gattc_write(conn_handle, &params);
    return err_code_convert(errno);
}

/*
 * @brief   Write value by handle without response
 * @param   [in] conn_handle: connection handle
 *          [in] handle: handle to the attribute to be written.
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
mible_status_t mible_gattc_write_cmd(uint16_t conn_handle, uint16_t handle,
    uint8_t* p_value, uint8_t len)
{
    uint32_t errno;
    ble_gattc_write_params_t params = {0};
    params.write_op = BLE_GATT_OP_WRITE_CMD;
    params.handle   = handle;
    params.p_value  = p_value;
    params.len      = len;
    
    errno = sd_ble_gattc_write(conn_handle, &params);
    return err_code_convert(errno);
}

/*TIMER related function*/
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

static app_timer_t* acquire_time()
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

static int release_timer(void* timer_id)
{
    for (uint8_t i = 0; i < TIMER_MAX_NUM; i++) {
        if (timer_id == &m_timer_pool[i].data) {
            m_timer_pool[i].is_avail = 1;
            return i;
        }
    }
    return -1;
}



/*
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
    mible_timer_handler timeout_handler,
    mible_timer_mode mode)
{
    uint32_t errno;
    
    app_timer_id_t id  = acquire_time();
    if (id == NULL)
        return MI_ERR_NO_MEM;

    app_timer_mode_t m = mode == MIBLE_TIMER_SINGLE_SHOT ? APP_TIMER_MODE_SINGLE_SHOT : APP_TIMER_MODE_REPEATED;
    app_timer_timeout_handler_t handler = timeout_handler;
    errno = app_timer_create(&id, m, handler);
    *p_timer_id = id;
    return err_code_convert(errno);
}

/*
 * @brief   Delete a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void * timer_id) 
{
    uint32_t errno;
    int id = release_timer(timer_id);
    if (id == -1)
        return MI_ERR_INVALID_PARAM;

    errno = app_timer_stop((app_timer_id_t)timer_id);
    return err_code_convert(errno);
}

/*
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
    uint32_t errno;
    errno = app_timer_start((app_timer_id_t)timer_id, APP_TIMER_TICKS(timeout_value, APP_TIMER_PRESCALER), p_context);
    return err_code_convert(errno);
}

/*
 * @brief   Stop a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully stopped.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *
 * */
mible_status_t mible_timer_stop(void* timer_id) 
{
    uint32_t errno;
    errno = app_timer_stop((app_timer_id_t)timer_id);
    return err_code_convert(errno); 
}

/* FLASH related function*/

/*
 * @brief   Create a record in flash 
 * @param   [in] record_id: identify a record in flash 
 *          [in] len: record length
 * @return  MI_SUCCESS              Create successfully.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_NO_MEM           Not enough flash memory to be assigned 
 *              
 * */
mible_status_t mible_record_create(uint16_t record_id, uint8_t len)
{
    static uint8_t fds_has_init = 0;

    if (fds_has_init == 0) {
        fds_has_init = 1;
        mi_psm_init();
    }
    
    // actually it does not need record_id and len.

    return MI_SUCCESS;  
}


/*
 * @brief   Delete a record in flash
 * @param   [in] record_id: identify a record in flash  
 * @return  MI_SUCCESS              Delete successfully. 
 *          MI_ERR_INVALID_PARAM   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
    uint32_t errno;
    errno = mi_psm_record_delete(record_id);
    return err_code_convert(errno);
}

/*
 * @brief   Restore data to flash
 * @param   [in] record_id: identify an area in flash
 *          [out] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAM   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data,
    uint8_t len)
{
    uint32_t errno;
    errno = mi_psm_record_read(record_id, p_data, len);
    return err_code_convert(errno);
}

/*
 * @brief   Store data to flash
 * @param   [in] record_id: identify an area in flash
 *          [in] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 * allowed size.
 *          MI_ERR_INVALID_PARAM    p_data is not aligned to a 4 byte boundary.
 * @note    Should use asynchronous mode to implement this function.
 *          The data to be written to flash has to be kept in memory until the
 * operation has terminated, i.e., an event is received.
 *          When record writing complete , call mible_arch_event_callback function and pass MIBLE_ARCH_EVT_RECORD_WRITE_CMP event and result. 
 * */
mible_status_t mible_record_write(uint16_t record_id, uint8_t* p_data,
    uint8_t len)
{
    uint32_t errno;
    errno = mi_psm_record_write(record_id, p_data, len);
    return err_code_convert(errno);
}

/*
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
    while(NRF_SUCCESS != sd_rand_application_vector_get(p_buf, len));
    return MI_SUCCESS;
}

/*
 * @brief   Encrypts a block according to the specified parameters. 128-bit
 * AES encryption.
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

    return err_code_convert(errno);
}


/* TASK schedulor related function  */
typedef struct {
    mible_handler_t handler;
    void *arg;
} mible_task_t;

NRF_QUEUE_DEF(mible_task_t, task_queue, 4, NRF_QUEUE_MODE_OVERFLOW);

/*
 * @brief   Post a task to a task quene, which can be executed in a right place(maybe a task in RTOS or while(1) in the main function).
 * @param   [in] handler: a pointer to function 
 *          [in] param: function parameters 
 * @return  MI_SUCCESS              Successfully put the handler to quene.      
 *          MI_ERR_NO_MEM           The task quene is full. 
 *          MI_ERR_INVALID_PARAM    Handler is NULL
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

/*
 * @brief   Function for executing all enqueued tasks.
 *
 * @note    This function must be called from within the main loop. It will 
 * execute all events scheduled since the last time it was called.
 * */
void mible_tasks_exec(void)
{
    uint32_t errno = 0;
    mible_task_t task;
    while(!errno) {
        errno = nrf_queue_pop(&task_queue, &task);
        if (errno == NRF_SUCCESS)
            task.handler(task.arg);
    }
}

/* IIC related function */
const nrf_drv_twi_t TWI0 = NRF_DRV_TWI_INSTANCE(0);
static mible_handler_t m_iic_handler;
static void twi0_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    iic_event_t event;
    switch (p_event->type) {
    case NRF_DRV_TWI_EVT_DONE:
        event = IIC_EVT_XFER_DONE;
        break;
    case NRF_DRV_TWI_EVT_ADDRESS_NACK:
        event = IIC_EVT_ADDRESS_NACK;
        break;
    case NRF_DRV_TWI_EVT_DATA_NACK:
        event = IIC_EVT_DATA_NACK;
        break;
    }
    m_iic_handler(&event);
}

/*
 * @brief   Function for initializing the IIC driver instance.
 * @param   [in] p_config: Pointer to the initial configuration.
 *          [in] handler: Event handler provided by the user. 
 * @return  MI_SUCCESS              Initialized successfully.
 *          MI_ERR_INVALID_PARAM    p_config or handler is a NULL pointer.
 *              
 * */
mible_status_t mible_iic_init(const iic_config_t * p_config, mible_handler_t handler)
{
    uint32_t errno;
    
    if (p_config == NULL || handler == NULL) {
        return MI_ERR_INVALID_PARAM;
    } else {
        m_iic_handler = handler;
    }

    const nrf_drv_twi_config_t msc_config = {
       .scl                = p_config->scl_pin,
       .sda                = p_config->sda_pin,
       .frequency          = p_config->freq == IIC_100K ? NRF_TWI_FREQ_100K : NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = true
    };

    errno = nrf_drv_twi_init(&TWI0, &msc_config, twi0_handler, NULL);
    MI_ERR_CHECK(errno);

    nrf_drv_twi_enable(&TWI0);
    
    return err_code_convert(errno);
}

/*
 * @brief   Function for uninitializing the IIC driver instance.
 * 
 *              
 * */
void mible_iic_uninit(void)
{
    nrf_drv_twi_uninit(&TWI0);
}

/*
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
mible_status_t mible_iic_tx(uint8_t addr, uint8_t * p_out, uint16_t len, bool no_stop)
{
    uint32_t errno;
    if (p_out == NULL)
        return MI_ERR_INVALID_PARAM;

    errno = nrf_drv_twi_tx(&TWI0, addr, p_out, len, no_stop);
    MI_ERR_CHECK(errno);
    return err_code_convert(errno);
}

/*
 * @brief   Function for reciving data to a IIC slave.
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
    uint32_t errno;
    if (p_in == NULL)
        return MI_ERR_INVALID_PARAM;

    errno = nrf_drv_twi_rx(&TWI0, addr, p_in, len);
    MI_ERR_CHECK(errno);
    return err_code_convert(errno);
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
    return nrf_gpio_pin_read(pin);
}
