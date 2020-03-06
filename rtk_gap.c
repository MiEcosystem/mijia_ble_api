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
#include "platform_list.h"


#if MIBLE_API_SYNC
#define RTK_GAP_TASK_TYPE_SCAN                         0
#define RTK_GAP_TASK_TYPE_ADV                          1
#define RTK_GAP_TASK_TYPE_UPDATE_ADV_PARAM             2

typedef struct
{
    bool scan_enable;
    mible_gap_scan_type_t scan_type;
    mible_gap_scan_param_t scan_param;
} rtk_gap_task_scan_t;

typedef struct
{
    bool adv_enable;
    mible_gap_adv_param_t adv_param;
} rtk_gap_task_adv_t;

#define RTK_GAP_MAX_ADV_DATA_LEN                      31
typedef struct
{
    uint8_t adv_data[RTK_GAP_MAX_ADV_DATA_LEN + 2];
    uint8_t adv_data_len;
    uint8_t scan_rsp_data[RTK_GAP_MAX_ADV_DATA_LEN + 2];
    uint8_t scan_rsp_data_len;
} rtk_gap_task_update_adv_param_t;

typedef struct _rtk_gap_task_t
{
    struct _rtk_gap_task_t *pnext;
    uint8_t task_type;
    union
    {
        rtk_gap_task_scan_t scan;
        rtk_gap_task_adv_t adv;
        rtk_gap_task_update_adv_param_t update_adv_param;
    };
} rtk_gap_task_t;

static rtk_gap_task_t *pcur_task;
static plt_list_t rtk_gap_task_list;
static void *rtk_gap_timer;
#define RTK_GAP_OPERATE_TIMEOUT            500
#endif

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

static T_GAP_CAUSE rtk_gap_scan_start(mible_gap_scan_type_t scan_type,
                                        mible_gap_scan_param_t scan_param);
static T_GAP_CAUSE rtk_gap_adv_start(mible_gap_adv_param_t *p_param);
static T_GAP_CAUSE rtk_gap_adv_data_set(uint8_t const *p_data,
                                 uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen);

#if MIBLE_API_SYNC
void rtk_gap_task_run(rtk_gap_task_t *ptask);

static void rtk_gap_cur_task_done(void)
{
    //MI_LOG_DEBUG("task done: 0x%x", pcur_task);
    /* stop timer first */
    mible_timer_stop(rtk_gap_timer);
    
    /* free current task */
    if (NULL != pcur_task)
    {
        plt_free(pcur_task, RAM_TYPE_DATA_ON);
        pcur_task = NULL;
    }
    /* run next task */
    rtk_gap_task_t *ptask = plt_list_pop(&rtk_gap_task_list);
    //MI_LOG_DEBUG("pop task: 0x%x", ptask);
    if (NULL != ptask)
    {
        rtk_gap_task_run(ptask);
    }
}

static void rtk_gap_timeout_handle(void *pargs)
{
    /* current task run timeout, maybe error happened or message missed */
    MI_LOG_ERROR("task operate timeout!");
    rtk_gap_cur_task_done();
}

void rtk_gap_task_run(rtk_gap_task_t *ptask)
{
    //MI_LOG_DEBUG("run task: 0x%x", ptask);
    if (NULL == rtk_gap_timer)
    {
        mible_timer_create(&rtk_gap_timer, rtk_gap_timeout_handle, MIBLE_TIMER_SINGLE_SHOT);
    }
    
    pcur_task = ptask;
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    switch (ptask->task_type)
    {
    case RTK_GAP_TASK_TYPE_ADV:
        if (ptask->adv.adv_enable)
        {
            MI_LOG_DEBUG("sync start adv");
            ret = rtk_gap_adv_start(&ptask->adv.adv_param);
            if (GAP_CAUSE_SUCCESS != ret)
            {
                MI_LOG_ERROR("sync start adv failed: %d", ret);
                rtk_gap_cur_task_done();
            }
            else
            {
                mible_timer_start(rtk_gap_timer, RTK_GAP_OPERATE_TIMEOUT, NULL);
            }
        }
        else
        {
            MI_LOG_DEBUG("sync stop adv");
            ret = le_adv_stop();
            if (GAP_CAUSE_SUCCESS != ret)
            {
                MI_LOG_ERROR("sync stop adv failed: %d", ret);
                rtk_gap_cur_task_done();
            }
            else
            {
                mible_timer_start(rtk_gap_timer, RTK_GAP_OPERATE_TIMEOUT, NULL);
            }
        }
        break;
    case RTK_GAP_TASK_TYPE_SCAN:
        if (ptask->scan.scan_enable)
        {
            MI_LOG_DEBUG("sync start scan");
            ret = rtk_gap_scan_start(ptask->scan.scan_type, ptask->scan.scan_param);
            if (GAP_CAUSE_SUCCESS != ret)
            {
                MI_LOG_ERROR("sync start scan failed: %d", ret);
                rtk_gap_cur_task_done();
            }
            else
            {
                mible_timer_start(rtk_gap_timer, RTK_GAP_OPERATE_TIMEOUT, NULL);
            }
        }
        else
        {
            MI_LOG_DEBUG("sync stop scan");
            ret = le_scan_stop();
            if (GAP_CAUSE_SUCCESS != ret)
            {
                MI_LOG_ERROR("sync stop scan failed: %d", ret);
                rtk_gap_cur_task_done();
            }
            else
            {
                mible_timer_start(rtk_gap_timer, RTK_GAP_OPERATE_TIMEOUT, NULL);
            }
        }
        break;
    case RTK_GAP_TASK_TYPE_UPDATE_ADV_PARAM:
        MI_LOG_DEBUG("sync update adv param");
        ret = rtk_gap_adv_data_set(ptask->update_adv_param.adv_data, ptask->update_adv_param.adv_data_len, ptask->update_adv_param.scan_rsp_data, ptask->update_adv_param.scan_rsp_data_len);
        if (GAP_CAUSE_SUCCESS != ret)
        {
            MI_LOG_ERROR("sync update adv param failed: %d", ret);
            rtk_gap_cur_task_done();
        }
        else
        {
            mible_timer_start(rtk_gap_timer, RTK_GAP_OPERATE_TIMEOUT, NULL);
        }
        break;
    default:
        break;
    }
}

void rtk_gap_task_try(rtk_gap_task_t *ptask)
{
    if (NULL == pcur_task)
    {
        rtk_gap_task_run(ptask);
    }
    else
    {
        //MI_LOG_DEBUG("pending task: 0x%x", ptask);
        plt_list_push(&rtk_gap_task_list, ptask);
    }
}

void mible_gap_update_adv_param_done(void)
{
    if ((NULL != pcur_task) &&
        (RTK_GAP_TASK_TYPE_UPDATE_ADV_PARAM == pcur_task->task_type))
    {
        MI_LOG_DEBUG("sync update adv param success");
        rtk_gap_cur_task_done();
    }
}

void mible_gap_dev_state_change(T_GAP_DEV_STATE state)
{
    if (NULL != pcur_task)
    {
        if (RTK_GAP_TASK_TYPE_ADV == pcur_task->task_type)
        {
            if ((pcur_task->adv.adv_enable) &&
                (GAP_ADV_STATE_ADVERTISING == state.gap_adv_state))
            {
                /* start adv success */
                MI_LOG_DEBUG("sync start adv success");
                rtk_gap_cur_task_done();
            }
            else if ((!pcur_task->adv.adv_enable) &&
                     (GAP_ADV_STATE_IDLE == state.gap_adv_state))
            {
                /* stop adv success */
                MI_LOG_DEBUG("sync stop adv success");
                rtk_gap_cur_task_done();
            }
        }
        else if (RTK_GAP_TASK_TYPE_SCAN == pcur_task->task_type)
        {
            if ((pcur_task->scan.scan_enable) &&
                (GAP_SCAN_STATE_SCANNING == state.gap_scan_state))
            {
                /* start scan success */
                MI_LOG_DEBUG("sync start scan success");
                rtk_gap_cur_task_done();
            }
            else if ((!pcur_task->scan.scan_enable) &&
                     (GAP_SCAN_STATE_IDLE == state.gap_scan_state))
            {
                /* stop scan success */
                MI_LOG_DEBUG("sync stop scan success");
                rtk_gap_cur_task_done();
            }
        }
    }
}
#endif

mible_status_t mible_gap_address_get(mible_addr_t mac)
{
    T_GAP_CAUSE err = gap_get_param(GAP_PARAM_BD_ADDR, mac);
    return err_code_convert(err);
}

static T_GAP_CAUSE rtk_gap_scan_start(mible_gap_scan_type_t scan_type,
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
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    return le_scan_start();
}

mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
                                    mible_gap_scan_param_t scan_param)
{
    mible_status_t err = MI_SUCCESS;
#if MIBLE_API_SYNC
    rtk_gap_task_t *ptask = plt_malloc(sizeof(rtk_gap_task_t), RAM_TYPE_DATA_ON);
    if (NULL != ptask)
    {
        ptask->task_type = RTK_GAP_TASK_TYPE_SCAN;
        ptask->scan.scan_enable = TRUE;
        ptask->scan.scan_type = scan_type;
        ptask->scan.scan_param = scan_param;
        rtk_gap_task_try(ptask);
    }
    else
    {
        MI_LOG_ERROR("scan start failed: out of memory!");
        err = MI_ERR_NO_MEM;
    }
#else
    T_GAP_CAUSE ret = rtk_gap_scan_start(scan_type, scan_param);
    err = err_code_convert(ret);
#endif
    return err;
}

mible_status_t mible_gap_scan_stop(void)
{
    mible_status_t err = MI_SUCCESS;
#if MIBLE_API_SYNC
    rtk_gap_task_t *ptask = plt_malloc(sizeof(rtk_gap_task_t), RAM_TYPE_DATA_ON);
    if (NULL != ptask)
    {
        ptask->task_type = RTK_GAP_TASK_TYPE_SCAN;
        ptask->scan.scan_enable = FALSE;
        rtk_gap_task_try(ptask);
    }
    else
    {
        MI_LOG_ERROR("scan stop failed: out of memory!");
        err = MI_ERR_NO_MEM;
    }
#else
    T_GAP_CAUSE ret = le_scan_stop();
    err = err_code_convert(ret);
#endif
    return err;
}

static T_GAP_CAUSE rtk_gap_adv_start(mible_gap_adv_param_t *p_param)
{
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
		
    return le_adv_start();
}

mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
    mible_status_t err = MI_SUCCESS;
#if MIBLE_API_SYNC
    rtk_gap_task_t *ptask = plt_malloc(sizeof(rtk_gap_task_t), RAM_TYPE_DATA_ON);
    if (NULL != ptask)
    {
        ptask->task_type = RTK_GAP_TASK_TYPE_ADV;
        ptask->adv.adv_enable = TRUE;
        ptask->adv.adv_param = *p_param;
        rtk_gap_task_try(ptask);
    }
    else
    {
        MI_LOG_ERROR("adv start failed: out of memory!");
        err = MI_ERR_NO_MEM;
    }
#else
    T_GAP_CAUSE ret = rtk_gap_adv_start(p_param);
    err = err_code_convert(ret);
#endif
    return err;
}

static T_GAP_CAUSE rtk_gap_adv_data_set(uint8_t const *p_data,
                                 uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
    if ((NULL != p_data) && (dlen > 0))
    {
        le_adv_set_param(GAP_PARAM_ADV_DATA, dlen, (void *)p_data);
    }

    if ((NULL != p_sr_data) && (srdlen > 0))
    {
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, srdlen, (void *)p_sr_data);
    }

    return le_adv_update_param();
}

mible_status_t mible_gap_adv_data_set(uint8_t const *p_data,
                                      uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
    mible_status_t err = MI_SUCCESS;
#if MIBLE_API_SYNC
    rtk_gap_task_t *ptask = plt_malloc(sizeof(rtk_gap_task_t), RAM_TYPE_DATA_ON);
    if (NULL != ptask)
    {
        uint8_t len = (dlen > RTK_GAP_MAX_ADV_DATA_LEN) ? RTK_GAP_MAX_ADV_DATA_LEN : dlen;
        ptask->task_type = RTK_GAP_TASK_TYPE_UPDATE_ADV_PARAM;
        memcpy(ptask->update_adv_param.adv_data, p_data, len);
        ptask->update_adv_param.adv_data_len = len;

        len = (srdlen > RTK_GAP_MAX_ADV_DATA_LEN) ? RTK_GAP_MAX_ADV_DATA_LEN : srdlen;
        memcpy(ptask->update_adv_param.scan_rsp_data, p_sr_data, len);
        ptask->update_adv_param.scan_rsp_data_len = len;
        rtk_gap_task_try(ptask);
    }
    else
    {
        MI_LOG_ERROR("adv data set failed: out of memory!");
        err = MI_ERR_NO_MEM;
    }
#else
    T_GAP_CAUSE ret = rtk_gap_adv_data_set(p_data, dlen, p_sr_data, srdlen);
    err = err_code_convert(ret);
#endif
    return err;
}

mible_status_t mible_gap_adv_stop(void)
{
    mible_status_t err = MI_SUCCESS;
#if MIBLE_API_SYNC
    rtk_gap_task_t *ptask = plt_malloc(sizeof(rtk_gap_task_t), RAM_TYPE_DATA_ON);
    if (NULL != ptask)
    {
        ptask->task_type = RTK_GAP_TASK_TYPE_ADV;
        ptask->adv.adv_enable = FALSE;
        rtk_gap_task_try(ptask);
    }
    else
    {
        MI_LOG_ERROR("adv stop failed: out of memory!");
        err = MI_ERR_NO_MEM;
    }
#else
    T_GAP_CAUSE ret = le_adv_stop();
    err = err_code_convert(ret);
#endif
    return err;
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

