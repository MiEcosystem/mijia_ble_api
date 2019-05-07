/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_gap.c
* @brief     xiaomi ble gap api
* @details   Gap data types and functions.
* @author    hector_huang
* @date      2018-1-4
* @version   v1.0
* *********************************************************************************************************
*/
#include <string.h>
#include <gap.h>
#include <gap_scan.h>
#include <gap_adv.h>
#include <gap_conn_le.h>
#include <gap_msg.h>
#include "mible_api.h"
#include "mible_port.h"
#include "mible_type.h"
#define MI_LOG_MODULE_NAME "RTK_GAP"
#include "mible_log.h"
#include "platform_types.h"

#define ENABLE_RTK_MESH    1

static mible_status_t err_code_convert(T_GAP_CAUSE cause)
{
    mible_status_t status;
    switch (cause)
    {
    case GAP_CAUSE_SUCCESS:
        status = MI_SUCCESS;
        break;
    case GAP_CAUSE_ALREADY_IN_REQ:
        status = MI_ERR_BUSY;
        break;
    case GAP_CAUSE_INVALID_STATE:
        status = MI_ERR_INVALID_STATE;
        break;
    case GAP_CAUSE_INVALID_PARAM:
        status = MI_ERR_INVALID_PARAM;
        break;
    case GAP_CAUSE_NON_CONN:
        status = MIBLE_ERR_INVALID_CONN_HANDLE;
        break;
    case GAP_CAUSE_NOT_FIND_IRK:
        status = MIBLE_ERR_UNKNOWN;
        break;
    case GAP_CAUSE_ERROR_CREDITS:
        status = MIBLE_ERR_UNKNOWN;
        break;
    case GAP_CAUSE_SEND_REQ_FAILED:
        status = MIBLE_ERR_UNKNOWN;
        break;
    case GAP_CAUSE_NO_RESOURCE:
        status = MI_ERR_RESOURCES;
        break;
    case GAP_CAUSE_INVALID_PDU_SIZE:
        status = MI_ERR_INVALID_LENGTH;
        break;
    case GAP_CAUSE_NOT_FIND:
        status = MI_ERR_NOT_FOUND;
        break;
    case GAP_CAUSE_CONN_LIMIT:
        status = MIBLE_ERR_UNKNOWN;
        break;
    case GAP_CAUSE_NO_BOND:
        status = MIBLE_ERR_UNKNOWN;
        break;
    case GAP_CAUSE_ERROR_UNKNOWN:
        status = MIBLE_ERR_UNKNOWN;
        break;
    default:
        status = MIBLE_ERR_UNKNOWN;
        break;
    }

    return status;
}

mible_status_t mible_gap_address_get(mible_addr_t mac)
{
    T_GAP_CAUSE err = gap_get_param(GAP_PARAM_BD_ADDR, mac);
    return err_code_convert(err);
}

#if !ENABLE_RTK_MESH
mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
                                    mible_gap_scan_param_t scan_param)
{
    T_GAP_CAUSE err = GAP_CAUSE_SUCCESS;
    uint8_t scan_mode;
    if (MIBLE_SCAN_TYPE_PASSIVE == scan_type)
    {
        scan_mode = GAP_SCAN_MODE_PASSIVE;
    }
    else
    {
        scan_mode = GAP_SCAN_MODE_ACTIVE;
    }

    le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
    uint16_t scan_window = scan_param.scan_window;
    uint16_t scan_interval = scan_param.scan_interval;
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    err = le_scan_start();
    return err_code_convert(err);
}

mible_status_t mible_gap_scan_stop(void)
{
    T_GAP_CAUSE err = le_scan_stop();
    return err_code_convert(err);
}

mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
    T_GAP_CAUSE err = GAP_CAUSE_SUCCESS;
    uint8_t  adv_evt_type;
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint16_t adv_int_min = p_param->adv_interval_min;
    uint16_t adv_int_max = p_param->adv_interval_max;

    if (MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED == p_param->adv_type)
    {
        adv_evt_type = GAP_ADTYPE_ADV_IND;
    }
    else if (MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED == p_param->adv_type)
    {
        adv_evt_type = GAP_ADTYPE_ADV_SCAN_IND;
    }
    else
    {
        adv_evt_type = GAP_ADTYPE_ADV_NONCONN_IND;
    }

    if (p_param->ch_mask.ch_37_off)
    {
        adv_chann_map &= ~GAP_ADVCHAN_37;
    }

    if (p_param->ch_mask.ch_38_off)
    {
        adv_chann_map &= ~GAP_ADVCHAN_38;
    }

    if (p_param->ch_mask.ch_39_off)
    {
        adv_chann_map &= ~GAP_ADVCHAN_39;
    }

    /* set advertising parameters */
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);

    err = le_adv_start();

    return err_code_convert(err);
}

mible_status_t mible_gap_adv_data_set(uint8_t const *p_data,
                                      uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
    T_GAP_CAUSE err = GAP_CAUSE_SUCCESS;
    if (NULL != p_data)
    {
        le_adv_set_param(GAP_PARAM_ADV_DATA, dlen, (void *)p_data);
    }

    if (NULL != p_sr_data)
    {
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, srdlen, (void *)p_sr_data);
    }

    err = le_adv_update_param();

    return err_code_convert(err);
}

mible_status_t mible_gap_adv_stop(void)
{
    T_GAP_CAUSE err = le_adv_stop();
    return err_code_convert(err);
}
#endif

mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
                                 mible_gap_connect_t conn_param)
{
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    T_GAP_REMOTE_ADDR_TYPE bd_type;

    conn_req_param.scan_interval = scan_param.scan_interval;
    conn_req_param.scan_window = scan_param.scan_window;
    conn_req_param.conn_interval_min = conn_param.conn_param.min_conn_interval;
    conn_req_param.conn_interval_max = conn_param.conn_param.max_conn_interval;
    conn_req_param.conn_latency = conn_param.conn_param.slave_latency;
    conn_req_param.supv_tout = conn_param.conn_param.conn_sup_timeout;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (MIBLE_ADDRESS_TYPE_PUBLIC == conn_param.type)
    {
        bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    }
    else
    {
        bd_type = GAP_REMOTE_ADDR_LE_RANDOM;
    }

    le_connect(0, conn_param.peer_addr, bd_type, GAP_LOCAL_ADDR_LE_PUBLIC, scan_param.timeout);

    return MI_SUCCESS;
}

mible_status_t mible_gap_disconnect(uint16_t conn_handle)
{
    T_GAP_CAUSE err = le_disconnect(conn_handle);
    return err_code_convert(err);
}

mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
                                            mible_gap_conn_param_t conn_params)
{
    uint16_t ce_len_min = 2 * (conn_params.min_conn_interval - 1);
    uint16_t ce_len_max = 2 * (conn_params.max_conn_interval - 1);
    T_GAP_CAUSE err = le_update_conn_param(conn_handle, conn_params.min_conn_interval,
                                           conn_params.max_conn_interval,
                                           conn_params.slave_latency,
                                           conn_params.conn_sup_timeout, ce_len_min, ce_len_max);

    return err_code_convert(err);
}

