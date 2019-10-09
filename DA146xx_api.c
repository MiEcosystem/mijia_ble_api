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
#include "mi_config.h"
#include "mible_api.h"
#include "mible_type.h"

#include "mible_port.h"
#define MI_LOG_MODULE_NAME "MI APP"
#include "mible_log.h"
#include "aes.h"

#include "osal.h"
#include "ble_att.h"
#include "ble_common.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "ble_uuid.h"
#include "ble_service.h"
#include <appl_storage.h>

#include "mi_psm.h"
#include "common/mible_beacon.h"

#include "ad_nvms.h"

#include "cryptography/mi_mesh_otp_config.h"

static mible_status_t err_code_convert(ble_error_t errno)
{
        mible_status_t stat;

        switch (errno) {
        case BLE_STATUS_OK:
            stat = MI_SUCCESS;
            break;
        case BLE_ERROR_NOT_FOUND:
            stat = MI_ERR_NOT_FOUND;
            break;
        case BLE_ERROR_INS_RESOURCES:
            stat = MI_ERR_NO_MEM;
            break;
        case BLE_ERROR_INVALID_PARAM:
            stat = MI_ERR_INVALID_PARAM;
            break;
        case BLE_ERROR_BUSY:
            stat = MI_ERR_BUSY;
            break;
        case BLE_ERROR_TIMEOUT:
            stat = MI_ERR_TIMEOUT;
            break;
        default:
                OS_ASSERT(0);
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
    own_address_t address;

    ble_gap_address_get(&address);
    memcpy(mac, address.addr, sizeof(address.addr));

    return MI_SUCCESS;
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
    //Will implement this after Central mode scenario get identified.
    //ble_gap_scan_start();
    OS_ASSERT(0);
    return MI_SUCCESS;
}

/*
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void)
{
    //Will implement this after Central mode scenario get identified.
    //ble_gap_scan_stop();

    OS_ASSERT(0);
    return MI_SUCCESS;
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
    ble_error_t ret;
    gap_conn_mode_t adv_type;

    switch (p_adv_param->adv_type) {
    case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:
           adv_type = GAP_CONN_MODE_UNDIRECTED;
           break;
    case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:
            // The original purpose of SCANNABLE UNDIRECTED should be start ADV after connection
            // It's not allow any coming connection in some case
            // This is not needed usually
            OS_ASSERT(0);
            return MI_ERR_INVALID_PARAM;
           break;
    case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:
            adv_type = GAP_CONN_MODE_NON_CONN;
           break;
    default:
            OS_ASSERT(0);
            return MI_ERR_INVALID_PARAM;
           break;
    }

    ble_gap_adv_intv_set(p_adv_param->adv_interval_min, p_adv_param->adv_interval_max);
    ret = ble_gap_adv_start(adv_type);

    return err_code_convert(ret);
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
    ble_error_t ret;

    ret = ble_gap_adv_data_set(dlen, p_data, srdlen, p_sr_data);
    return err_code_convert(ret);
}

/*
 * @brief   Stop advertising
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
ble_error_t global_ret;

mible_status_t mible_gap_adv_stop(void)
{
    ble_error_t ret = 0;

    global_ret = ble_gap_adv_stop();
    return err_code_convert(ret);
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
    ble_error_t ret;

    OS_ASSERT(0);
    return err_code_convert(ret);
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
    ble_error_t ret;

    ret = ble_gap_disconnect(conn_handle, BLE_HCI_ERROR_REMOTE_USER_TERM_CON);
    return err_code_convert(ret);
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
    ble_error_t ret;

    gap_conn_params_t cp;

    cp.interval_min = conn_params.min_conn_interval;
    cp.interval_max = conn_params.max_conn_interval;
    cp.slave_latency = conn_params.slave_latency;
    cp.sup_timeout = conn_params.conn_sup_timeout;

    ret = ble_gap_conn_param_update(conn_handle, &cp);

    return err_code_convert(ret);
}

static uint8_t base_uuid[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                                                0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static void compact_uuid(att_uuid_t *uuid)
{
        if (uuid->type == ATT_UUID_16) {
                // nothing to compact
                return;
        }

        if (memcmp(uuid->uuid128, base_uuid, 12) || memcmp(&uuid->uuid128[14], &base_uuid[14], 2)) {
                // not a BT UUID
                return;
        }

        uuid->type = ATT_UUID_16;
        uuid->uuid16 = uuid->uuid128[13] << 8 | uuid->uuid128[12];
}

void MI_UUID_Translate(const mible_uuid_t *pmiuuid, att_uuid_t *pauuid)
{
        pauuid->type = (att_uuid_type_t)pmiuuid->type;
        if (pauuid->type == ATT_UUID_16)
            pauuid->uuid16 = pmiuuid->uuid16;
        else
            memcpy(pauuid->uuid128, pmiuuid->uuid128, ATT_UUID_LENGTH);

        compact_uuid(pauuid);
}

uint16_t calc_desc_num(mible_gatts_srv_db_t *p_service)
{
        uint16_t num_desc = 0;

        for (uint8_t idx = 0; idx < p_service->char_num; idx++) {
                mible_gatts_char_db_t *p_char = p_service->p_char_db + idx;
                if ((p_char->char_property & MIBLE_NOTIFY) || (p_char->char_property & MIBLE_INDICATE))
                        num_desc++;
        }

        return num_desc;
}

void update_handles(mible_gatts_srv_db_t *p_service, uint16_t start_h)
{
        p_service->srv_handle = start_h;
        for(uint8_t i = 0; i < p_service->char_num; i++) {
                mible_gatts_char_db_t *p_char = p_service->p_char_db + i;
                p_char->char_value_handle += start_h;
        }
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
    uint8_t i;
    att_uuid_t uuid;
    uint16_t num_attr, start_h, num_desc;
    ble_service_config_t *service_config = NULL;

    for (uint8_t idx = 0, max = p_server_db->srv_num; idx < max; idx++) {
            mible_gatts_srv_db_t *p_service = p_server_db->p_srv_db + idx;
            MI_UUID_Translate(&p_service->srv_uuid, &uuid);
            num_desc = calc_desc_num(p_service);
            num_attr = ble_service_get_num_attr(NULL, p_service->char_num, num_desc);
            ble_gatts_add_service(&uuid, GATT_SERVICE_PRIMARY, num_attr);

            for(i = 0; i < p_service->char_num; i++) {
                    mible_gatts_char_db_t *p_char = p_service->p_char_db + i;
                    /* make sure no extra char description defined */
                    //OS_ASSERT(!p_char->char_desc_db);

                    gatt_prop_t props = {0};
                    memcpy((uint8_t*)&props, &p_char->char_property, 1);

                    MI_UUID_Translate(&p_char->char_uuid, &uuid);
                    ble_gatts_add_characteristic(&uuid, props,
                                                    ble_service_config_elevate_perm(ATT_PERM_RW, service_config),
                                                    p_char->char_value_len, 0, NULL, &p_char->char_value_handle);
                    if ((props & GATT_PROP_NOTIFY) || (props & GATT_PROP_INDICATE)) {
                            ble_uuid_create16(UUID_GATT_CLIENT_CHAR_CONFIGURATION, &uuid);
                            ble_gatts_add_descriptor(&uuid, ATT_PERM_RW, sizeof(uint16_t), 0,
                                                        NULL);
                    }
            }

            ble_gatts_register_service(&start_h, 0);
            update_handles(p_service, start_h);
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
    /*
    uint16_t len_full = 20;
    uint8_t* p_fullvalue;


    ble_gatts_get_value(value_handle, &len_full, NULL);
    OS_ASSERT(len_full);
    p_fullvalue = OS_MALLOC(len_full);
    ble_gatts_get_value(value_handle, &len_full, p_fullvalue);
    memcpy(p_fullvalue + offset, p_value, len);
    */
    OS_ASSERT(!offset);
    ble_gatts_set_value(value_handle, len, p_value);
    //OS_FREE(p_fullvalue);

    return MI_SUCCESS;
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
    uint16_t len_full = 0;
    ble_gatts_get_value(char_handle, &len_full, p_value);
    *p_len = len_full;

    return MI_SUCCESS;
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
        OS_ASSERT(0);
        return MI_SUCCESS;
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

    if (type == 1)
            ble_gatts_send_event(conn_handle, char_value_handle, GATT_EVENT_NOTIFICATION, len, p_value);
    else
            ble_gatts_send_event(conn_handle, char_value_handle, GATT_EVENT_INDICATION, len, p_value);
    return MI_SUCCESS;
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
    OS_ASSERT(0);
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
    OS_ASSERT(0);
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
    OS_ASSERT(0);
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
    OS_ASSERT(0);
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

    OS_ASSERT(0);
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

    OS_ASSERT(0);
    return err_code_convert(errno);
}


#define TIMER_MAX_NUM     4

#define DEFAULT_INTEVAL_MS 100

/* TIMER related function */
typedef struct {
    uint8_t is_avail;
    uint32_t timeout_value;
    OS_TIMER handle;
    mible_timer_handler handler;
} timer_item_t;

timer_item_t m_timer_pool[TIMER_MAX_NUM] = {
    [0] = { .is_avail = 1},
    [1] = { .is_avail = 1},
    [2] = { .is_avail = 1},
    [3] = { .is_avail = 1}
};

uint8_t acquire_time()
{
    uint8_t i;
    for (i = 0; i < TIMER_MAX_NUM; i++) {
        if (m_timer_pool[i].is_avail) {
            m_timer_pool[i].is_avail = 0;
            return i;
        }
    }
    return -1;
}

static int release_timer(void* timer_id)
{
    for (uint8_t i = 0; i < TIMER_MAX_NUM; i++) {
        if (timer_id == m_timer_pool[i].handle) {
            m_timer_pool[i].is_avail = 1;
            return i;
        }
    }
    return -1;
}

extern void mi_timer_handler(OS_TIMER timer);
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
    uint32_t i = acquire_time();
    OS_TIMER timer_handle;
    
    if (i < 0)
    {
        OS_ASSERT(0);
        return MI_ERR_INVALID_PARAM;
    }

    timer_handle = OS_TIMER_CREATE("test", OS_MS_2_TICKS(DEFAULT_INTEVAL_MS), mode,
                                            (void *) i, mi_timer_handler);

    m_timer_pool[i].handler = timeout_handler;
    m_timer_pool[i].handle = timer_handle;
    *p_timer_id = timer_handle;

    return MI_SUCCESS;
}

/*
 * @brief   Delete a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void * timer_id) 
{
    int id = release_timer(timer_id);
    if (id == -1)
        return MI_ERR_INVALID_PARAM;

    OS_TIMER_DELETE(timer_id, OS_TIMER_FOREVER);
    return MI_SUCCESS;
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
    OS_TIMER_CHANGE_PERIOD(timer_id, OS_MS_2_TICKS(timeout_value), OS_TIMER_FOREVER);
    OS_TIMER_START(timer_id, OS_TIMER_FOREVER);
    return MI_SUCCESS;
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

    OS_TIMER_STOP(timer_id, OS_TIMER_FOREVER);

    return MI_SUCCESS;
}

void mitimer_callback(uint8_t index)
{
        m_timer_pool[index].handler(NULL);
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
    OS_ASSERT(0);
    appl_storage_drop_item(record_id);

    return MI_SUCCESS;
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
    uint8_t* p_tmpdata;
    uint32_t tmp_len;

    //OS_ASSERT(0);
    if (appl_storage_get_item(record_id, (void **)&p_tmpdata, (size_t *)&tmp_len) == APPL_STORAGE_NOT_FOUND)
    {
            //OS_ASSERT(0);
            return MI_ERR_INVALID_PARAM;
    } else {
            if (len >= tmp_len)
                memcpy(p_data, p_tmpdata, tmp_len);
            else
                OS_ASSERT(0);
    }

    return MI_SUCCESS;
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
    //OS_ASSERT(0);

    appl_storage_put_item(record_id, p_data, len, true);

    return MI_SUCCESS;
}


extern void sys_trng_get_bytes(uint8_t *buffer, size_t size);
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
    sys_trng_get_bytes(p_buf, len);
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
    uint8_t tmpPlain[16];
    uint8_t tmpCipher[16];
    mbedtls_aes_context aes_ctx;

    if (key == NULL || plaintext == NULL || ciphertext == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    if (plen > 16) {
        return MI_ERR_INVALID_LENGTH;
    }

    mbedtls_aes_init(&aes_ctx);
    memset(tmpPlain, 0, 16);
    memset(tmpCipher, 0, 16);

    memcpy(tmpPlain, plaintext, plen);
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, tmpPlain, tmpCipher);

    memcpy(ciphertext, tmpCipher, 16);

    return MI_SUCCESS;
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
    OS_ASSERT(0);
    return err_code_convert(errno);
}

/*
 * @brief   Function for uninitializing the IIC driver instance.
 * 
 *              
 * */
void mible_iic_uninit(void)
{
        OS_ASSERT(0);
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
    OS_ASSERT(0);
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
    OS_ASSERT(0);
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
        OS_ASSERT(0);
        return 0;
}

nvms_t otp_key_partition;

mible_status_t mible_nvm_init()
{
        otp_key_partition = ad_nvms_open(NVMS_LOG_PART);

        return MI_SUCCESS;
}

mible_status_t mible_nvm_write(void * buffer, uint32_t length, uint32_t offset)
{
        ad_nvms_write(otp_key_partition, offset - (uint32_t)POTP_BASE, (uint8_t *)buffer, length);
        return MI_SUCCESS;
}

mible_status_t mible_nvm_read(void * buffer, uint32_t length, uint32_t offset)
{
        ad_nvms_read(otp_key_partition, offset - (uint32_t)POTP_BASE, (uint8_t *)buffer, length);

        return MI_SUCCESS;
}

mible_status_t mible_upgrade_firmware()
{
        OS_ASSERT(0);
        return MI_SUCCESS;
}

#if 0
int mi_mesh_otp_read(uint16_t item_type, uint8_t *p_out, uint16_t max_olen)
{
        //OS_ASSERT(0);

        return 0;
}
#endif

/*
void advertising_init(void)
{
    MI_LOG_INFO("advertising init...\n");
    mible_addr_t dev_mac;
    mibeacon_frame_ctrl_t frame_ctrl = {
            .mesh = 1,
            .secure_auth = 1,
            .version = 5
    };

    mibeacon_capability_t cap = {
            .connectable = 1,
            .encryptable = 1,
            .bondAbility = 1 };

    mibeacon_mesh_t mesh = {
            .pb_gatt = 1,
            .state   = 0,
    };

    mible_gap_address_get(dev_mac);

    mibeacon_config_t mibeacon_cfg = {
    .frame_ctrl = frame_ctrl,
    .pid = 0x0379,
    .p_mac = (mible_addr_t*) dev_mac,
    .p_capability =  &cap,
    .p_obj = NULL,
    .p_mesh = &mesh,
    };

    uint8_t service_data[31-3];
    uint8_t service_data_len=0;

    if(MI_SUCCESS != mible_service_data_set(&mibeacon_cfg, service_data, &service_data_len)){
        MI_LOG_ERROR("mibeacon_data_set failed. \r\n");
        return;
    }

    mible_gap_adv_data_set(service_data, service_data_len,NULL,0);

    MI_LOG_INFO("adv mi service data:");
    MI_LOG_HEXDUMP(service_data, service_data_len);

    mible_gap_adv_param_t adv_param =(mible_gap_adv_param_t){
        .adv_type = MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
        .adv_interval_min = BLE_ADV_INTERVAL_FROM_MS(200),
        .adv_interval_max = BLE_ADV_INTERVAL_FROM_MS(200),
    };
    mible_gap_adv_start(&adv_param);
    MI_LOG_PRINTF("\r\n");
        return;
}

*/
