/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_os.c
* @brief     xiaomi ble os api
* @details   OS data types and functions.
* @author    hector_huang
* @date      2018-1-4
* @version   v1.0
* *********************************************************************************************************
*/
#include "mible_type.h"
#include "platform_os.h"
#include "platform_types.h"
#define MI_LOG_MODULE_NAME "RTK_OS"
#include "mible_log.h"
#include "rtk_common.h"

#define DEFAULT_TIME_INTERVAL   0xFFFFFF
#define MAX_TIMER_CONTEXT       20
#define USER_TIMER_INDEX        10

typedef struct
{
    plt_timer_t timer;
    mible_timer_handler timeout_handler;
    void *pcontext;
} timer_context_t;

static timer_context_t timer_context[MAX_TIMER_CONTEXT];

static void mi_timeout_handler(void *timer)
{
    T_IO_MSG msg;
    msg.type = MIBLE_API_MSG_TYPE_TIMEOUT;
    msg.u.buf = timer;
    mible_api_inner_msg_send(&msg);
}

void mible_handle_timeout(void *timer)
{
    for (uint8_t idx = 0; idx < MAX_TIMER_CONTEXT; ++idx)
    {
        if (timer == timer_context[idx].timer)
        {
            if (NULL != timer_context[idx].timeout_handler)
            {
                timer_context[idx].timeout_handler(timer_context[idx].pcontext);
            }
            break;
        }
    }
}

mible_status_t mible_timer_create(void **p_timer_id,
                                  mible_timer_handler timeout_handler, mible_timer_mode mode)
{
    if (NULL == p_timer_id)
    {
        return MI_ERR_INVALID_PARAM;
    }

    uint8_t idx;
    for (idx = 0; idx < MAX_TIMER_CONTEXT; ++idx)
    {
        if (NULL == timer_context[idx].timer)
        {
            break;
        }
    }

    if (idx >= MAX_TIMER_CONTEXT)
    {
        return MI_ERR_NO_MEM;
    }

    *p_timer_id = plt_timer_create("mi", DEFAULT_TIME_INTERVAL,
                                   (mode == MIBLE_TIMER_SINGLE_SHOT) ? FALSE : TRUE,
                                   0, mi_timeout_handler);
    if (NULL == *p_timer_id)
    {
        return MI_ERR_RESOURCES;
    }

    timer_context[idx].timer = *p_timer_id;
    timer_context[idx].timeout_handler = timeout_handler;

    return MI_SUCCESS;
}

mible_status_t mible_user_timer_create(void **p_timer_id,
                    mible_timer_handler timeout_handler, mible_timer_mode mode)
{
    if (NULL == p_timer_id)
    {
        return MI_ERR_INVALID_PARAM;
    }

    uint8_t idx;
    for (idx = USER_TIMER_INDEX; idx < MAX_TIMER_CONTEXT; ++idx)
    {
        if (NULL == timer_context[idx].timer)
        {
            break;
        }
    }

    if (idx >= MAX_TIMER_CONTEXT)
    {
        return MI_ERR_NO_MEM;
    }

    *p_timer_id = plt_timer_create("mi", DEFAULT_TIME_INTERVAL,
                                   (mode == MIBLE_TIMER_SINGLE_SHOT) ? FALSE : TRUE,
                                   0, mi_timeout_handler);
    if (NULL == *p_timer_id)
    {
        return MI_ERR_RESOURCES;
    }

    timer_context[idx].timer = *p_timer_id;
    timer_context[idx].timeout_handler = timeout_handler;

    return MI_SUCCESS;
}

mible_status_t mible_timer_delete(void *timer_id)
{
    if (NULL == timer_id)
    {
        return MI_ERR_INVALID_PARAM;
    }

    for (uint8_t idx = 0; idx < MAX_TIMER_CONTEXT; ++idx)
    {
        if (timer_context[idx].timer == timer_id)
        {
            timer_context[idx].timer = NULL;
            timer_context[idx].timeout_handler = NULL;
            timer_context[idx].pcontext = NULL;
            plt_timer_delete(timer_id, 0);
            return MI_SUCCESS;
        }
    }

    return MI_ERR_NOT_FOUND;
}

mible_status_t mible_timer_start(void *timer_id, uint32_t timeout_value,
                                 void *p_context)
{
    if (NULL == timer_id)
    {
        return MI_ERR_INVALID_PARAM;
    }

    for (uint8_t idx = 0; idx < MAX_TIMER_CONTEXT; ++idx)
    {
        if (timer_context[idx].timer == timer_id)
        {
            timer_context[idx].pcontext = p_context;
            plt_timer_change_period(timer_id, timeout_value, 0);
            return MI_SUCCESS;
        }
    }

    return MI_ERR_NOT_FOUND;
}

mible_status_t mible_timer_stop(void *timer_id)
{
    if (NULL == timer_id)
    {
        return MI_ERR_INVALID_PARAM;
    }

    for (uint8_t idx = 0; idx < MAX_TIMER_CONTEXT; ++idx)
    {
        if (timer_context[idx].timer == timer_id)
        {
            plt_timer_stop(timer_id, 0);
            return MI_SUCCESS;
        }
    }

    return MI_ERR_NOT_FOUND;
}


