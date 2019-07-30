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
#include "mible_log.h"

#include <string.h>
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "hci.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "peripheral.h"
#include "gapbondmgr.h"
#include "log.h"
#include "global_config.h"
#include "aes.h"
#include "mijiaService.h"
#include "chipsea_func.h"

#if 0
#ifndef MIBLE_MAX_USERS
#define MIBLE_MAX_USERS 4
#endif

/* GAP, GATTS, GATTC event callback function */
static uint8_t m_gap_users, m_gattc_users, m_gatts_users, m_arch_users;
static mible_gap_callback_t m_gap_cb_table[MIBLE_MAX_USERS];
static mible_gatts_callback_t m_gatts_cb_table[MIBLE_MAX_USERS];
static mible_gattc_callback_t m_gattc_cb_table[MIBLE_MAX_USERS];
static mible_arch_callback_t m_arch_cb_table[MIBLE_MAX_USERS];

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
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_gap_cb_table[user] != NULL) {
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
        if (m_gatts_cb_table[user] != NULL) {
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
    for (int user = 0; user < MIBLE_MAX_USERS; user++) {
        if (m_arch_cb_table[user] != NULL) {
            m_arch_cb_table[user](evt, param);
        }
    }
}
#endif

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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    osal_memcpy(mac, (void *)0x1fff11f9, 6);
    return MI_SUCCESS;
}

#if 0
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
__WEAK mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
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
__WEAK mible_status_t mible_gap_scan_stop(void)
{
    return MI_SUCCESS;
}
#endif

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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    if ((p_param->adv_interval_min < 0x0020) || (p_param->adv_interval_min > 0x4000))
    {
        return MI_ERR_INVALID_PARAM;
    }
    if ((p_param->adv_interval_max < 0x0020) || (p_param->adv_interval_max > 0x4000))
    {
        return MI_ERR_INVALID_PARAM;
    }
    if (p_param->adv_type > MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED)
    {
        return MI_ERR_INVALID_PARAM;
    }

    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, p_param->adv_interval_min); //设置受限制的最小广播间隔
    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, p_param->adv_interval_max); //设置受限制的最大广播间隔
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, p_param->adv_interval_min); //设置通用的最小广播间隔
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, p_param->adv_interval_max); //设置通用的最大广播间隔

    uint8 advType = GAP_ADTYPE_ADV_IND; //设置广播类型为可连接的非定向广播
    if (p_param->adv_type == MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED)
    {
        advType = GAP_ADTYPE_ADV_SCAN_IND; //设置广播类型为可扫描的非定向广播
    }
    else if (p_param->adv_type == MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED)
    {
        advType = GAP_ADTYPE_ADV_NONCONN_IND; //设置广播类型为不可连接的非定向广播
    }
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8), &advType); //设置广播类型，非定向可连接广播

    uint8 advChnMap = GAP_ADVCHAN_37 | GAP_ADVCHAN_38 | GAP_ADVCHAN_39; //默认打开所有广播通道
    if (p_param->ch_mask.ch_37_off == 1)
    {
        advChnMap ^= GAP_ADVCHAN_37;
    }
    if (p_param->ch_mask.ch_38_off == 1)
    {
        advChnMap ^= GAP_ADVCHAN_38;
    }
    if (p_param->ch_mask.ch_39_off == 1)
    {
        advChnMap ^= GAP_ADVCHAN_39;
    }
    GAPRole_SetParameter(GAPROLE_ADV_CHANNEL_MAP, sizeof(uint8), &advChnMap); //设置广播通道

    // device starts advertising upon initialization
    uint8 initial_advertising_enable = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &initial_advertising_enable); //打开广播

    // By setting this to zero, the device will go into the waiting state after
    // being discoverable for 30.72 second, and will not being advertising again
    // until the enabler is set back to TRUE
    uint16 gapRole_AdvertOffTime = 0;
    GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(uint16), &gapRole_AdvertOffTime); //设置广播时间，设置为0，则广播30s，如果需要连续广播，则需要在停止广播后重新开启

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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    char advDataBuf[31];
    char scanRspDataBuf[31];

    if ((dlen > 31) || (srdlen > 31))
    {
        return MI_ERR_INVALID_PARAM;
    }
    if (p_data != NULL)
    {
        memset(advDataBuf, 0, sizeof(advDataBuf));
        memcpy(advDataBuf, p_data, dlen);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, dlen, advDataBuf); //设置广播数据
    }
    if (p_sr_data != NULL)
    {
        memset(scanRspDataBuf, 0, sizeof(scanRspDataBuf));
        memcpy(scanRspDataBuf, p_sr_data, srdlen);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, srdlen, scanRspDataBuf); //设置扫描应答数据
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    uint8 initial_advertising_enable = FALSE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &initial_advertising_enable); //关闭广播
    return MI_SUCCESS;
}

#if 0
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
__WEAK mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
        mible_gap_connect_t conn_param)
{
    return MI_SUCCESS;
}
#endif

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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    GAPRole_TerminateConnection();
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    static uint8_t connParaInitFlag = 0;

    if ((conn_params.min_conn_interval < 0x0006) || (conn_params.min_conn_interval > 0x0C80))
    {
        return MI_ERR_INVALID_PARAM;
    }
    if ((conn_params.max_conn_interval < 0x0006) || (conn_params.max_conn_interval > 0x0C80))
    {
        return MI_ERR_INVALID_PARAM;
    }
    if (conn_params.slave_latency > 0x01F3)
    {
        return MI_ERR_INVALID_PARAM;
    }
    if ((conn_params.conn_sup_timeout < 0x000A) || (conn_params.conn_sup_timeout > 0x0C80))
    {
        return MI_ERR_INVALID_PARAM;
    }

    if(connParaInitFlag == 0){
        // Connection Pause Peripheral time value (in seconds)
        #define DEFAULT_CONN_PAUSE_PERIPHERAL 6                                                //手机连接设备到设备发送连接参数请求之间的时间，时间太小会存在手机的兼容性问题
        VOID GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL); //设置手机连接设备到设备发送连接参数请求之间的时间

        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &(conn_params.min_conn_interval)); //设置最小连接间隔
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16), &(conn_params.max_conn_interval)); //设置最大连接间隔
        GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16), &(conn_params.slave_latency));         //设置潜伏次数
        GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16), &(conn_params.conn_sup_timeout)); //设置连接超时时间

        uint8 enable_update_request = TRUE;
        GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(uint8), &enable_update_request); //使能或禁止连接参数更新请求
        connParaInitFlag = 1;
    }
    else{
        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &(conn_params.min_conn_interval)); //设置最小连接间隔
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16), &(conn_params.max_conn_interval)); //设置最大连接间隔
        GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16), &(conn_params.slave_latency));         //设置潜伏次数
        GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16), &(conn_params.conn_sup_timeout)); //设置连接超时时间

		gaprole_States_t gapstate = GAPROLE_INIT;
		GAPRole_GetParameter(GAPROLE_STATE, &gapstate);
		if (gapstate == GAPROLE_CONNECTED){
            uint8 sendUpdateFlag = TRUE;
            GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_REQ, sizeof( uint8 ), &sendUpdateFlag );
        }
    }
    return MI_SUCCESS;
}

/**
 *        GATT Server APIs
 */

mible_status_t mible_update_db_para(uint16_t char_uuid, uint8_t char_property, uint8_t *char_data, uint8_t char_data_len)
{
    gattAttribute_t *mible_att_db = MijiaProfile_GetAttrDB();
    for (int ii = 0; ii < MIJIA_SERVICE_ARR_SIZE; ii++)
    {
        uint16_t uuid = BUILD_UINT16(mible_att_db[ii].type.uuid[0], mible_att_db[ii].type.uuid[1]);
        if ((uuid == char_uuid) && (ii > 0))
        {
            if (char_property != *(mible_att_db[ii - 1].pValue))
            {
                MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
                return MI_ERR_INVALID_PARAM;
            }
            else
            {
                if (char_data_len != MijiaProfile_GetLenByIdx(ii))
                {
                    MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
                    return MI_ERR_INVALID_PARAM;
                }
                else
                {
                    memcpy(mible_att_db[ii].pValue, char_data, char_data_len);
                    return MI_SUCCESS;
                }
            }
        }
    }
    MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
    return MI_ERR_INVALID_PARAM;
}

mible_status_t mible_update_app_handle(mible_gatts_db_t *p_server_db)
{
    gattAttribute_t *mible_att_db = MijiaProfile_GetAttrDB();

    for (uint8_t idx = 0; idx < p_server_db->srv_num; idx++)
    {
        mible_gatts_srv_db_t *p_service = (p_server_db->p_srv_db + idx);
        if (p_service == NULL)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_ADDR;
        }
        if (p_service->srv_type != MIBLE_PRIMARY_SERVICE)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_PARAM;
        }
        if (p_service->srv_uuid.type != 0)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_PARAM;
        }
        if (p_service->srv_uuid.uuid16 != 0xFE95)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_PARAM;
        }
        p_service->srv_handle = mible_att_db[0].handle;

        for (int i = 0; i < p_service->char_num; i++)
        {
            mible_gatts_char_db_t *p_char = &p_service->p_char_db[i];
            if (p_char == NULL)
            {
                return MI_ERR_INVALID_ADDR;
            }
            if (p_char->char_uuid.type != 0)
            {
                MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
                return MI_ERR_INVALID_PARAM;
            }
            uint16_t char_uuid16 = p_char->char_uuid.uuid16;
            p_char->char_value_handle = MijiaProfile_GetHadleByUuid(char_uuid16);
        }
        return MI_SUCCESS;
    }
    MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
    return MI_ERR_INVALID_PARAM;
}

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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    if ((p_server_db == NULL))
    {
        MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
        return MI_ERR_INVALID_ADDR;
    }
    if ((p_server_db->p_srv_db == NULL))
    {
        MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
        return MI_ERR_INVALID_ADDR;
    }

	// MI_LOG_DEBUG("server number:%d\n",p_server_db->srv_num);
    for (uint8_t srvIdx = 0; srvIdx < p_server_db->srv_num; srvIdx++)
    {
        mible_gatts_srv_db_t *p_service = (p_server_db->p_srv_db + srvIdx);
        // MI_LOG_DEBUG("\tsrv_type:%d\n",p_service->srv_type);
        // MI_LOG_DEBUG("\tsrv_handle:%d\n",p_service->srv_handle);
        // MI_LOG_DEBUG("\tsrv_uuid_type:%d\n",p_service->srv_uuid.type);
        // MI_LOG_DEBUG("\tsrv_uuid_uuid16:%d,0x%04X\n",p_service->srv_uuid.uuid16,p_service->srv_uuid.uuid16);
        // MI_LOG_DEBUG("\tchar_num:%d\n",p_service->char_num);
        // MI_LOG_DEBUG("\tchar_db:0x%X\n",p_service->p_char_db);
		// MI_LOG_PRINTF("-------------------------------------------\n");

        if (p_service == NULL)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_ADDR;
        }
        if (p_service->srv_type != MIBLE_PRIMARY_SERVICE)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_PARAM;
        }
        if (p_service->srv_uuid.type != 0)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_PARAM;
        }
        if (p_service->srv_uuid.uuid16 != 0xFE95)
        {
            MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
            return MI_ERR_INVALID_PARAM;
        }

        for (int chrIdx = 0; chrIdx < p_service->char_num; chrIdx++)
        {
            mible_gatts_char_db_t *p_char = (p_service->p_char_db + chrIdx);

            // MI_LOG_DEBUG("\t\tchar_uuid_type:%d\n",p_char->char_uuid.type);
            // MI_LOG_DEBUG("\t\tchar_uuid_uuid16:%d\n",p_char->char_uuid.uuid16);
            // MI_LOG_DEBUG("\t\tchar_property:%d\n",p_char->char_property);
            // MI_LOG_DEBUG("\t\tchar_value_len:%d\n",p_char->char_value_len);

            if (p_char == NULL)
            {
                MI_LOG_ERROR("%s(%d):service init error ,chrIdx = %d\n", __func__, __LINE__,chrIdx);
                return MI_ERR_INVALID_ADDR;
            }
            if (p_char->char_uuid.type != 0)
            {
                MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
                return MI_ERR_INVALID_PARAM;
            }
            uint16_t char_uuid16 = p_char->char_uuid.uuid16;
            uint8_t char_property = p_char->char_property;
            uint8_t *char_value = p_char->p_value;
            uint8_t char_value_len = p_char->char_value_len;
            if (mible_update_db_para(char_uuid16, char_property, char_value, char_value_len) != MI_SUCCESS)
            {
                MI_LOG_ERROR("%s(%d):service init error \n", __func__, __LINE__);
                return MI_ERR_INVALID_PARAM;
            }
        }
    }

    MijiaProfile_AddService(MIJIA_SERVICE_BIT);
    mible_update_app_handle(p_server_db);

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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    if (SUCCESS == MijiaProfile_SetParameterByHandle(value_handle, p_value, len))
    {
        return MI_SUCCESS;
    }
    else
    {
        MI_LOG_ERROR("%s(%d):gatts value get error \n", __func__, __LINE__);
        return MI_ERR_INVALID_PARAM;
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
mible_status_t mible_gatts_value_get(uint16_t srv_handle,
        uint16_t value_handle, uint8_t* p_value, uint8_t *p_len)
{
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    if (SUCCESS == MijiaProfile_GetParameterByHandle(value_handle, p_value, p_len))
    {
        return MI_SUCCESS;
    }
    else
    {
        MI_LOG_ERROR("%s(%d):gatts value get error \n", __func__, __LINE__);
        return MI_ERR_INVALID_PARAM;
    }
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    if (SUCCESS == MijiaProfile_SetParameterByHandle(char_value_handle, p_value, len))
    {
        return MI_SUCCESS;
    }
    else
    {
        MI_LOG_ERROR("%s(%d):gatts value get error \n", __func__, __LINE__);
        return MI_ERR_INVALID_PARAM;
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
mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle,
        uint8_t status, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type)
{
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    return MI_SUCCESS;
}

/**
 *        GATT Client APIs
 */

#if 0
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
#endif

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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    return timer_create(p_timer_id, timeout_handler, mode);
}

/**
 * @brief   Delete a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void* timer_id)
{
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    return timer_delete(timer_id);
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    return timer_start((void *)timer_id, timeout_value, p_context);
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    return timer_stop(timer_id);
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
    // MI_LOG_DEBUG("%s(%d):record_id=%d,len=%d,\n",__func__,__LINE__,record_id,len);
    return record_create(record_id, len);
}

/**
 * @brief   Delete a record in flash
 * @param   [in] record_id: identify a record in flash  
 * @return  MI_SUCCESS              Delete successfully. 
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
    // MI_LOG_DEBUG("%s(%d):record_id=%d,\n",__func__,__LINE__,record_id);
    return record_delete(record_id);
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
    // MI_LOG_DEBUG("%s(%d):record_id=%d,dat = ",__func__,__LINE__,record_id);
    // LOG_HEX(p_data,len);
    // LOG("\n");
    mible_status_t sta = record_read(record_id, p_data, len);
    // MI_LOG_DEBUG("%s(%d):record_id=%d,dat = ",__func__,__LINE__,record_id);
    // LOG_HEX(p_data,len);
    // LOG("\n");
    return sta;
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
mible_status_t mible_record_write(uint16_t record_id, const uint8_t* p_data,
        uint8_t len)
{
    // MI_LOG_DEBUG("%s(%d):record_id=%d,dat = ",__func__,__LINE__,record_id);
    // LOG_HEX(p_data,len);
    // LOG("\n");

    mible_arch_evt_param_t param;
    param.record.id = record_id;
    param.record.status = record_write(record_id, p_data, len);
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);
    return MI_SUCCESS;
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    int ii = 0;
    for (ii = 0; ii < (len >> 1); ii++)
    {
        uint16_t rand16 = osal_rand();
        p_buf[(ii * 2) + 0] = (rand16 >> 8) & 0xff;
        p_buf[(ii * 2) + 1] = (rand16 >> 0) & 0xff;
    }
    if ((len & 0x01) == 0x01)
    {
        uint16_t rand16 = osal_rand();
        p_buf[(ii * 2) + 0] = (rand16 >> 8) & 0xff;
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    struct AES_ctx ctx;
    uint8_t plainData[16] = {0};

    if ((key == NULL) || (plaintext == NULL))
    {
        return MI_ERR_INVALID_ADDR;
    }
    if (plen > 16)
    {
        return MI_ERR_INVALID_LENGTH;
    }
    AES_init_ctx(&ctx, key);
		memcpy((uint8_t *)plainData,plaintext, plen);
		AES_ECB_encrypt(&ctx, plainData);
		memcpy(ciphertext,plainData, plen);
	
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
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    return task_post(handler, arg);
}

/**
 * @brief   Function for executing all enqueued tasks.
 *
 * @note    This function must be called from within the main loop. It will 
 * execute all events scheduled since the last time it was called.
 * */
void mible_tasks_exec(void)
{
    // MI_LOG_DEBUG("%s(%d)\n",__func__,__LINE__);
    tasks_exec();
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
mible_status_t mible_iic_init(const iic_config_t *p_config,
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
mible_status_t mible_iic_rx(uint8_t addr, uint8_t *p_in, uint16_t len)
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

mible_status_t mible_nvm_init(void)
{
    return MI_SUCCESS;
}

/**
 * @brief   Function for reading data from Non-Volatile Memory.
 * @param   [out] p_data:  Pointer to data to be restored.
 *          [in] length:   Data size in bytes.
 *          [in] address:  Address in Non-Volatile Memory to read.
 * @return  MI_ERR_INTERNAL:  invalid NVM address.
 *          MI_SUCCESS
 * */
mible_status_t mible_nvm_read(void *p_data, uint32_t length, uint32_t address)
{
    return MI_ERR_BUSY;
}

/**
 * @brief   Writes data to Non-Volatile Memory.
 * @param   [in] p_data:   Pointer to data to be stored.
 *          [in] length:   Data size in bytes.
 *          [in] address:  Start address used to store data.
 * @return  MI_ERR_INTERNAL:  invalid NVM address.
 *          MI_SUCCESS
 * */
mible_status_t mible_nvm_write(void *p_data, uint32_t length, uint32_t address)
{
    return MI_ERR_BUSY;
}

mible_status_t mible_upgrade_firmware(void)
{
    return MI_ERR_BUSY;
}
