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
#include "gatt_client_interface.h"
#include "gatt_server_interface.h"
#include "gap_interface.h"
#include "ApplMain.h"
#include "TimersManager.h"
#include "SecLib.h"
#include "RNG_Interface.h"  
#include "ble_conn_manager.h"
#include "gatt_db_app_interface.h"
#include "nvds.h"
#include "MemManager.h"
#include "fsl_i2c.h"
#include "fsl_iocon.h"

/**
 *        GAP APIs
 */

/**
 * @brief   Get BLE mac address.
 * @param   [out] mac: pointer to data
 * @return  MI_SUCCESS          The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note: 	You should copy gap mac to mac[6]  
 * */
mible_status_t mible_gap_address_get(mible_addr_t mac) 
{
    FLib_MemCpy(mac, gBleDeviceAddress, sizeof(mible_addr_t));
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
    gapScanningParameters_t     ScanParams;

    ScanParams.type = (bleScanType_t)scan_type;
    ScanParams.interval = scan_param.scan_interval;
    ScanParams.window = scan_param.scan_window;
    ScanParams.ownAddressType = gBleAddrTypePublic_c;
    ScanParams.filterPolicy = gScanAll_c;
    return (mible_status_t)(App_StartScanning(&ScanParams, NULL, true));
}

/**
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void) 
{
    return (mible_status_t)(Gap_StopScanning());
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
    uint8_t map;
    gAdvParams.minInterval = p_param->adv_interval_min;
    gAdvParams.maxInterval = p_param->adv_interval_max;
    gAdvParams.advertisingType = (bleAdvertisingType_t)(p_param->adv_type);

    map  = (!p_param->ch_mask.ch_37_off) << 0;
    map |= (!p_param->ch_mask.ch_38_off) << 1;
    map |= (!p_param->ch_mask.ch_39_off) << 2;
    gAdvParams.channelMap = (gapAdvertisingChannelMapFlags_t)map;
    
    gAdvParams.ownAddressType = gBleAddrTypePublic_c;
    gAdvParams.filterPolicy = gProcessAll_c;
    return (mible_status_t)(Gap_SetAdvertisingParameters(&gAdvParams));
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
    uint8_t idx = 0;
    uint8_t numAd = 0;
    
    while(idx < dlen)
    {
        gAppAdvertisingData.aAdStructures[numAd].length = p_data[idx];
        gAppAdvertisingData.aAdStructures[numAd].adType = (gapAdType_t)p_data[idx+1];
        gAppAdvertisingData.aAdStructures[numAd].aData  = (uint8_t*)p_data + idx + 2;
        
        idx += gAppAdvertisingData.aAdStructures[numAd].length + 1;
        numAd++;
        
        if(numAd >= gAppAdvertisingData.cNumAdStructures)
        {
            break;
        }
    }
    gAppAdvertisingData.cNumAdStructures = numAd;
    
    idx = 0;
    numAd = 0;
    while(idx < srdlen)
    {
        gAppScanRspData.aAdStructures[numAd].length = p_sr_data[idx];
        gAppScanRspData.aAdStructures[numAd].adType = (gapAdType_t)p_sr_data[idx+1];
        gAppScanRspData.aAdStructures[numAd].aData  = (uint8_t*)p_sr_data + idx + 2;
        
        idx += gAppScanRspData.aAdStructures[numAd].length + 1;
        numAd++;
        
        if(numAd >= gAppScanRspData.cNumAdStructures)
        {
            break;
        }
    }
    gAppScanRspData.cNumAdStructures = numAd;
    
    BleConnManager_GapPeripheralConfig();
    
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
    return (mible_status_t)(Gap_StopAdvertising()); 
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
    gapConnectionRequestParameters_t    gConnReqParams;
    
    FLib_MemCpy(gConnReqParams.peerAddress, conn_param.peer_addr, sizeof(bleDeviceAddress_t));
    gConnReqParams.peerAddressType = (bleAddressType_t)(conn_param.type);
    gConnReqParams.connIntervalMin = conn_param.conn_param.min_conn_interval;
    gConnReqParams.connIntervalMax = conn_param.conn_param.max_conn_interval;
    gConnReqParams.connLatency = conn_param.conn_param.slave_latency;
    gConnReqParams.supervisionTimeout = conn_param.conn_param.conn_sup_timeout;
    gConnReqParams.filterPolicy = gUseDeviceAddress_c;
    gConnReqParams.connEventLengthMin = 0;
    gConnReqParams.connEventLengthMax = 0xFF;
    gConnReqParams.ownAddressType = gBleAddrTypePublic_c;
    
    gConnReqParams.scanInterval = scan_param.scan_interval;
    gConnReqParams.scanWindow = scan_param.scan_window;
    

    return (mible_status_t)(App_Connect(&gConnReqParams, NULL));
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
    return (mible_status_t)(Gap_Disconnect(conn_handle)); 
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
    return (mible_status_t)((Gap_UpdateConnectionParameters(conn_handle, conn_params.min_conn_interval,
            conn_params.max_conn_interval, conn_params.slave_latency, conn_params.conn_sup_timeout,
            0x00,0xffff)));
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
 
#define MIBLE_SRV_UUID                         	0XFE95//{0x95,0xFE}  
#define MIBLE_STD_CHAR_NUM                      7
#define MIBLE_CHAR_UUID_TOKEN                  	0x0001
#define MIBLE_CHAR_UUID_PRODUCTID              	0x0002
#define MIBLE_CHAR_UUID_VERSION                	0x0004
#define MIBLE_CHAR_UUID_WIFICFG                	0x0005
#define MIBLE_CHAR_UUID_AUTHENTICATION         	0x0010
#define MIBLE_CHAR_UUID_DID                    	0x0013
#define MIBLE_CHAR_UUID_BEACONKEY              	0x0014
#define MIBLE_CHAR_UUID_DEVICE                 	0x0015
#define MIBLE_CHAR_UUID_SECURE                 	0x0016


mible_gatts_char_db_t char_db[] =
{
    {
        .char_uuid = 
        {
            .type = 0,
            .uuid16 = MIBLE_CHAR_UUID_TOKEN,
        },
        .char_property = gPermissionFlagWritable_c,
        .p_value = NULL,
        .char_value_len = 0,
    },
    
    {
    .char_uuid = 
        {
            .type = 0,
            .uuid16 = MIBLE_CHAR_UUID_PRODUCTID,
        },
        .char_property = gPermissionFlagReadable_c,
        .p_value = "\x11\x22",
        .char_value_len = 2,
    },
    
    {
        .char_uuid = 
        {
            .type = 0,
            .uuid16 = MIBLE_CHAR_UUID_VERSION,
        },
        .char_property = gPermissionFlagReadable_c,
        .p_value = NULL,
        .char_value_len = 0,
    },
    {
        .char_uuid = 
        {
            .type = 0,
            .uuid16 = MIBLE_CHAR_UUID_WIFICFG,
        },
        .char_property = gPermissionFlagWritable_c,
        .p_value = NULL,
        .char_value_len = 0,
    },
    {
        .char_uuid = 
        {
            .type = 0,
            .uuid16 = MIBLE_CHAR_UUID_AUTHENTICATION,
        },
        .char_property = gPermissionFlagWritable_c,
        .p_value = NULL,
        .char_value_len = 0,
    },
    {
        .char_uuid = 
        {
            .type = 0,
            .uuid16 = MIBLE_CHAR_UUID_DID,
        },
        .char_property = (gPermissionFlagWritable_c | gPermissionFlagReadable_c),
        .p_value = NULL,
        .char_value_len = 0,
    },
    {
        .char_uuid = 
        {
            .type = 0,
            .uuid16 = MIBLE_CHAR_UUID_BEACONKEY,
        },
        .char_property = gPermissionFlagReadable_c,
        .p_value = NULL,
        .char_value_len = 0,
    },
};

mible_gatts_srv_db_t srv_db[] =
{
    {
        .srv_type = MIBLE_PRIMARY_SERVICE,
        .srv_handle = NULL,
        .srv_uuid = 
                    {
                        .type = 0,
                        .uuid16 = MIBLE_SRV_UUID,
                    },
        .char_num = MIBLE_STD_CHAR_NUM, 
        .p_char_db = char_db
    }
};

mible_gatts_db_t gatts_db =
{
    .p_srv_db = srv_db,
    .srv_num = 1
};

#define MAX_HANDLE_FOR_WRITENOTIFICATIONS  10
uint16_t HandlesForWriteNotifications[MAX_HANDLE_FOR_WRITENOTIFICATIONS];
uint8_t NumOfHandlesForWriteNotifications = 0;
mible_status_t mible_gatts_service_init(mible_gatts_db_t *p_server_db)
{
    uint8_t srvNum = 0;
    uint8_t charNum = 0;
    mible_gatts_srv_db_t *svr_db;
    mible_gatts_char_db_t *char_db;
    
    for(srvNum = 0; srvNum < p_server_db->srv_num; srvNum++)
    {
        svr_db = &(p_server_db->p_srv_db[srvNum]);
        GattDb_FindServiceHandle(0x0001, svr_db->srv_uuid.type == 1 ? gBleUuidType128_c : gBleUuidType16_c, 
                                    (bleUuid_t*)&svr_db->srv_uuid.uuid16, &svr_db->srv_handle);
        
        char_db = svr_db->p_char_db;
        for(charNum = 0; charNum < svr_db->char_num; charNum++)
        {
            char_db = &(svr_db->p_char_db[charNum]);
            GattDb_FindCharValueHandleInService(svr_db->srv_handle, char_db->char_uuid.type == 1 ? gBleUuidType128_c : gBleUuidType16_c,
                                                    (bleUuid_t*)&(char_db->char_uuid.uuid16), &char_db->char_value_handle);
            GattDb_WriteAttribute(char_db->char_value_handle, char_db->char_value_len, char_db->p_value);
            
            if(char_db->char_property & MIBLE_WRITE)
            {
                if(NumOfHandlesForWriteNotifications < NumberOfElements(HandlesForWriteNotifications))
                {
                    HandlesForWriteNotifications[NumOfHandlesForWriteNotifications] = char_db->char_value_handle;
                    NumOfHandlesForWriteNotifications++;
                }
            }
        }
    }
    
    static mible_arch_evt_param_t param;
    FLib_MemSet(&param, 0, sizeof(param));
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
mible_status_t mible_gatts_value_set(uint16_t srv_handle, uint16_t value_handle,
    uint8_t offset, uint8_t* p_value,
    uint8_t len)
{
    bleResult_t result;
    
    if(offset != 0)
        return MI_ERR_INVALID_PARAM;

    /* Update characteristic value */
    result = GattDb_WriteAttribute(value_handle, len, p_value);
    return (mible_status_t)result;
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
mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len)
{
    bleResult_t result;
    /* Get characteristic value */
    result = GattDb_ReadAttribute(value_handle, 0xFFFF, p_value, (uint16_t*)p_len);
    return (mible_status_t)result;
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
mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
    uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
    uint8_t len, uint8_t type)
{
    bool_t isNotifActive;
    bleResult_t result;
    uint16_t  handleCccd;
    
    /* Get handle of CCCD */
    if ((result = GattDb_FindCccdHandleForCharValueHandle(char_value_handle, &handleCccd)) != gBleSuccess_c)
        return (mible_status_t)result; 

    if(type == 1)
    {
        result = Gap_CheckNotificationStatus(conn_handle, handleCccd, &isNotifActive);
        if ((gBleSuccess_c == result) && (TRUE == isNotifActive))
            result = GattServer_SendInstantValueNotification(conn_handle, char_value_handle, len, p_value);
        else
            return MI_ERR_INVALID_STATE;
    }
    else if(type == 2)
    {
        result = Gap_CheckIndicationStatus(conn_handle, handleCccd, &isNotifActive);
        if ((gBleSuccess_c == result) && (TRUE == isNotifActive))
            result = GattServer_SendInstantValueIndication(conn_handle, char_value_handle, len, p_value);
        else
            return MI_ERR_INVALID_STATE;
    }

    return (mible_status_t)result;
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
    uint16_t length = len;
    
    if (status == 1) {
        
        if (type == 1) {
            GattDb_ReadAttribute(char_value_handle, 0xFFFF, p_value, &length);
            GattServer_SendAttributeWrittenStatus(conn_handle, char_value_handle, gAttErrCodeNoError_c);
        } else {
            GattDb_WriteAttribute(char_value_handle, len, p_value);
            GattServer_SendAttributeWrittenStatus(conn_handle, char_value_handle, gAttErrCodeNoError_c);
        }
    } else {
        if (type == 1) {
            GattServer_SendAttributeReadStatus(conn_handle, char_value_handle, gAttErrCodeReadNotPermitted_c);
        } else {
            GattServer_SendAttributeWrittenStatus(conn_handle, char_value_handle, gAttErrCodeWriteNotPermitted_c);
        }
    }
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
mible_status_t mible_gattc_primary_service_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range, mible_uuid_t* p_srv_uuid)
{
    #if 0
		gattService_t * aOutPrimaryServices;
		if(handle_range.begin_handle)
		{
				return (mible_status_t)(GattClient_DiscoverAllPrimaryServices(conn_handle, aOutPrimaryServices, NULL, NULL));
		}else
		{
			bleUuid_t srv_uuid;
			srv_uuid.uuid16 = p_srv_uuid->uuid16;
			FLib_MemCpy(srv_uuid.uuid128, (p_srv_uuid->uuid128), sizeof(p_srv_uuid->uuid128));
			return (mible_status_t)(GattClient_DiscoverPrimaryServicesByUuid(conn_handle, NULL, &srv_uuid, NULL, NULL, NULL));
		}
    #endif
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
mible_status_t
mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range, mible_uuid_t* p_char_uuid)
{
    #if 0
		bleUuid_t srv_uuid;
		srv_uuid.uuid16 = p_char_uuid->uuid16;
		FLib_MemCpy(srv_uuid.uuid128, (p_char_uuid->uuid128), sizeof(p_char_uuid->uuid128));
    return (mible_status_t)(GattClient_DiscoverCharacteristicOfServiceByUuid(conn_handle, gBleUuidType16_c, &srv_uuid, NULL, NULL, NULL, NULL));
    #endif
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
mible_status_t
mible_gattc_clt_cfg_descriptor_discover(uint16_t conn_handle,
    mible_handle_range_t handle_range)
{
    #if 0
    return (mible_status_t)(GattClient_DiscoverAllCharacteristicDescriptors(conn_handle, NULL, handle_range.end_handle, NULL));
    #endif
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
    #if 0
    gattAttribute_t IODescriptor;
	
		FLib_MemCpy(IODescriptor.uuid.uuid128, (p_char_uuid->uuid128), sizeof(p_char_uuid->uuid128));
		IODescriptor.uuid.uuid16 = p_char_uuid->uuid16;
		IODescriptor.handle = handle_range.begin_handle;
    return (mible_status_t)(GattClient_ReadCharacteristicDescriptor(conn_handle, &IODescriptor, 512));
    #endif
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
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle, uint16_t att_handle,
    uint8_t* p_value, uint8_t len)
{
    #if 0
    return (mible_status_t)(GattClient_WriteCharacteristicValue(conn_handle, (gattCharacteristic_t *)att_handle, len, p_value, 1, 0, 0, NULL));
    #endif
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
mible_status_t mible_gattc_write_cmd(uint16_t conn_handle, uint16_t att_handle,
    uint8_t* p_value, uint8_t len)
{
    #if 0
    return (mible_status_t)(GattClient_WriteCharacteristicValue(conn_handle, (gattCharacteristic_t *)(att_handle), len, p_value, 1, 0, 0, NULL));
    #endif
    return MI_SUCCESS;
}

/**
 *        SOFT TIMER APIs
 */


typedef struct {
    tmrTimerID_t id;
    pfTmrCallBack_t cb;
    tmrTimerType_t type;    
}mible_timer_t;

#define MIBLE_TIMER_MAX  gTmrApplicationTimers_c
mible_timer_t mible_timer[MIBLE_TIMER_MAX];

uint8_t find_exist_timer(tmrTimerID_t id)
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
    mible_timer_handler timeout_handler,
    mible_timer_mode mode)
{
    uint8_t index;
    tmrTimerID_t p_id;

    p_id = TMR_AllocateTimer();
    if(p_id == gTmrInvalidTimerID_c)
        return MI_ERR_INVALID_PARAM;
    
    index = find_empty_timer();
    if(index < MIBLE_TIMER_MAX)
    {
        mible_timer[index].id = p_id;
        mible_timer[index].cb = (pfTmrCallBack_t)timeout_handler;
        mible_timer[index].type =  (mode == MIBLE_TIMER_SINGLE_SHOT) ? gTmrLowPowerSingleShotMillisTimer_c : gTmrLowPowerIntervalMillisTimer_c ;
        
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
    
    index = find_exist_timer((tmrTimerID_t)timer_id);
    if(index < MIBLE_TIMER_MAX)
    {
        TMR_FreeTimer((tmrTimerID_t)timer_id);
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
    tmrErrCode_t result;
    uint8_t index;
    
    index = find_exist_timer((tmrTimerID_t)timer_id);
    if(index < MIBLE_TIMER_MAX)
    {
        result = TMR_StartLowPowerTimer(mible_timer[index].id, mible_timer[index].type, timeout_value, mible_timer[index].cb, p_context);
    }
    else
    {
        return MI_ERR_INVALID_PARAM;
    }
     return (mible_status_t)result;
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
    TMR_StopTimer((tmrTimerID_t)timer_id);
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

    err = nvds_del(record_id);   //need to add 0x030254c5 A nvds_del to fw_symbols_mdk.h 
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

    err = nvds_put(record_id, len, p_data);

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
        RNG_GetRandomNo(&rngVal);
        *(uint32_t*)p_buf = rngVal;
        p_buf += 4;
    }
    RNG_GetRandomNo(&rngVal);
    for(int i = len%4; i > 0; i--)
    {
        *p_buf++ = (rngVal >> 8*i) &0xFF;
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
    const uint8_t* plaintext, uint8_t plen,
    uint8_t* ciphertext)
{
    uint8_t plain[16] = {0};
    uint8_t cipher[16];
    
    if( plaintext == NULL || key == NULL)
        return MI_ERR_INVALID_ADDR;
    else if (plen > 16)
        return MI_ERR_INVALID_LENGTH;
    
    FLib_MemCpy(plain, (void*)plaintext, plen);
    
    AES_128_Encrypt(plain, key, cipher);
    
    FLib_MemCpy(ciphertext, cipher, plen);
    
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
    App_PostCallbackMessage((appCallbackHandler_t)handler,(appCallbackParam_t)arg);
    return MI_SUCCESS;
} 


void mible_tasks_exec(void)
{
}

#include <stdio.h>
#include <stdarg.h>
extern uint8_t gAppSerMgrIf;

#define SHELL_CB_SIZE 64
uint16_t Log_Printf(char * format,...)
{
    va_list ap;
    uint16_t n = 0;
    char *pStr = (char*)MEM_BufferAlloc(SHELL_CB_SIZE);

    if (pStr)
    {
        va_start(ap, format);
        n = vsnprintf(pStr, SHELL_CB_SIZE, format, ap);
        //va_end(ap); /* follow MISRA... */
        Serial_AsyncWrite(gAppSerMgrIf, (uint8_t*)pStr, n, NULL, NULL);
        MEM_BufferFree(pStr);
    }

    return n;
}

void Log_Hexdump(uint8_t* hex, uint8_t len)
{
    Serial_PrintHex(gAppSerMgrIf, hex, len, gAllowToBlock_d);
}


/**
 *        IIC APIs
 */
#define I2C_MASTER_CLOCK_FREQUENCY (8000000)
#define I2C_MASTER_BASEADDR (I2C0)

static i2c_master_handle_t g_m_handle;
static mible_handler_t mible_iic_handler;

static void iic_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    iic_event_t event;
    /* Signal transfer success when received success status. */
    if (status == kStatus_Success)
    {
       event = IIC_EVT_XFER_DONE;
    }
    /* Signal transfer success when received success status. */
    if (status == kStatus_I2C_Nak)
    {
        event = IIC_EVT_DATA_NACK;
    }
    mible_iic_handler(&event);
}



/**
 * @brief   Function for initializing the IIC driver instance.
 * @param   [in] p_config: Pointer to the initial configuration.
 *          [in] handler: Event handler provided by the user. 
 * @return  MI_SUCCESS              Initialized successfully.
 *          MI_ERR_INVALID_PARAM    p_config or handler is a NULL pointer.
 *              
 * */
mible_status_t mible_iic_init(const iic_config_t * p_config, mible_handler_t handler)
{
    i2c_master_config_t masterConfig;
    
    if (p_config == NULL || handler == NULL) {
        return MI_ERR_INVALID_PARAM;
    }
    mible_iic_handler = handler;
    
    const uint32_t portA_pin6_config = (
        IOCON_FUNC4 |                                            /* Selects pin function 4 */
        IOCON_MODE_HIGHZ |                                       /* Selects High-Z function */
        IOCON_DRIVE_LOW                                          /* Enable low drive strength */
    );
    IOCON_PinMuxSet(IOCON, 0, 6, portA_pin6_config); /* PORTA PIN6 (coords: 5) is configured as FC1_RTS_SCL */
    const uint32_t portA_pin7_config = (
        IOCON_FUNC4 |                                            /* Selects pin function 4 */
        IOCON_MODE_HIGHZ |                                       /* Selects High-Z function */
        IOCON_DRIVE_LOW                                          /* Enable low drive strength */
    );
    IOCON_PinMuxSet(IOCON, 0, 7, portA_pin7_config); /* PORTA PIN7 (coords: 4) is configured as FC1_CTS_SDA */
    
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = (p_config->freq == IIC_100K) ? 100000 : 400000;

    I2C_MasterInit(I2C_MASTER_BASEADDR, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);
    I2C_MasterTransferCreateHandle(I2C_MASTER_BASEADDR, &g_m_handle, iic_master_callback, NULL);

    
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
mible_status_t mible_iic_tx(uint8_t addr, uint8_t * p_out, uint16_t len, bool no_stop)
{
    i2c_master_transfer_t masterXfer;
    status_t reVal = kStatus_Fail;
    
    if (p_out == NULL)
    {
        return MI_ERR_INVALID_PARAM;
    }
    
    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = p_out;
    masterXfer.dataSize = len;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    
    reVal = I2C_MasterTransferNonBlocking(I2C_MASTER_BASEADDR, &g_m_handle, &masterXfer);
    if (reVal == kStatus_I2C_Busy)
    {
        return MI_ERR_BUSY;
    }
    
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
    i2c_master_transfer_t masterXfer;
    status_t reVal = kStatus_Fail;
    
    if (p_in == NULL)
    {
        return MI_ERR_INVALID_PARAM;
    }
    
    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = addr;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = p_in;
    masterXfer.dataSize = len;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    
    reVal = I2C_MasterTransferNonBlocking(I2C_MASTER_BASEADDR, &g_m_handle, &masterXfer);
    if (reVal == kStatus_I2C_Busy)
    {
        return MI_ERR_BUSY;
    }
    
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
    return !I2C_MasterGetBusIdleState(I2C_MASTER_BASEADDR);
}
