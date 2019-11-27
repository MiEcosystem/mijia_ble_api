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
#include "mesh_api.h"
#include "rtk_common.h"

static uint8_t rtk_adv_data[31];
static uint8_t rtk_adv_data_len;
static gap_sched_adv_type_t rtk_adv_type;
static plt_timer_t rtk_adv_timer;

extern void rtk_gap_adv_timeout(void);

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

mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
                                    mible_gap_scan_param_t scan_param)
{
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
    gap_sched_params_set(GAP_SCHED_PARAMS_SCAN_WINDOW, &scan_window, sizeof(scan_window));
    gap_sched_params_set(GAP_SCHED_PARAMS_SCAN_INTERVAL, &scan_interval,
                         sizeof(scan_interval));
    gap_sched_scan(TRUE);
    
    return MI_SUCCESS;
}

mible_status_t mible_gap_scan_stop(void)
{
    gap_sched_scan(FALSE);
    
    return MI_SUCCESS;
}

mible_status_t mible_gap_adv_send(void)
{
    uint8_t *padv_data = gap_sched_task_get();
    if (NULL == padv_data)
    {
        MI_LOG_ERROR("send adv data failed: busy!");
        return MI_ERR_RESOURCES;
    }

    gap_sched_task_p ptask = CONTAINER_OF(padv_data, gap_sched_task_t, adv_data);
    memcpy(ptask->adv_data, rtk_adv_data, rtk_adv_data_len);
    ptask->adv_len = rtk_adv_data_len;
    ptask->adv_type = rtk_adv_type;

    gap_sched_try(ptask);
    
    //MI_LOG_DEBUG("adv data:");
    //MI_LOG_HEXDUMP(rtk_adv_data, rtk_adv_data_len);

    return MI_SUCCESS;
}

static void rtk_adv_timeout_handler(void *pargs)
{
    T_IO_MSG msg;
    msg.type = MIBLE_API_MSG_TYPE_ADV_TIMEOUT;
    mible_api_inner_msg_send(&msg);
}

mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
    /* ser adv type */
    if (MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED == p_param->adv_type)
    {
        rtk_adv_type = GAP_SCHED_ADV_TYPE_IND;
    }
    else if (MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED == p_param->adv_type)
    {
        rtk_adv_type = GAP_SCHED_ADV_TYPE_SCAN_IND;
    }
    else
    {
        rtk_adv_type = GAP_SCHED_ADV_TYPE_NONCONN_IND;
    }
    
    /* create timer to start adv */
    if (NULL == rtk_adv_timer)
    {
        rtk_adv_timer = plt_timer_create("adv_timer", p_param->adv_interval_min*625/1000, TRUE, 0, rtk_adv_timeout_handler);
        if (NULL == rtk_adv_timer)
        {
            return MI_ERR_RESOURCES;
        }
    }
    else
    {
        plt_timer_change_period(rtk_adv_timer, p_param->adv_interval_min*625/1000, 0);
    }
    
    if (!plt_timer_is_active(rtk_adv_timer))
    {
        plt_timer_start(rtk_adv_timer, 0);
    }

    return MI_SUCCESS;
}

mible_status_t mible_gap_adv_data_set(uint8_t const *p_data,
                                      uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
    T_GAP_CAUSE err = GAP_CAUSE_SUCCESS;
    if (NULL != p_data)
    {
        memcpy(rtk_adv_data, p_data, dlen);
        rtk_adv_data_len = dlen;
    }

    if (NULL != p_sr_data)
    {
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, srdlen, (void *)p_sr_data);
    }

    return err_code_convert(err);
}

mible_status_t mible_gap_adv_stop(void)
{
    if (NULL != rtk_adv_timer)
    {
        plt_timer_delete(rtk_adv_timer, 0);
        rtk_adv_timer = NULL;
    }
    return MI_SUCCESS;
}

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

