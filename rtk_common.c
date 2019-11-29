/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_common.c
* @brief     xiaomi ble common parameters
* @details   common data types and functions.
* @author    hector_huang
* @date      2019-11-18
* @version   v1.0
* *********************************************************************************************************
*/
#include <stdlib.h>
#include "rtk_common.h"
#include <os_msg.h>
#include <os_task.h>
#include "mible_api.h"
#define MI_LOG_MODULE_NAME "RTK_COMMON"
#include "mible_log.h"

static void *mible_api_event_queue;
static void *mible_api_io_queue;

static const uint8_t mible_api_event = EVENT_MI_BLEAPI;

extern void mible_handle_timeout(void *timer);
#if MIBLE_MESH_API
extern mible_status_t mible_gap_adv_send(void);
#endif

void mible_api_init(void *pevent_queue, void *pio_queue)
{
    mible_api_event_queue = pevent_queue;
    mible_api_io_queue = pio_queue;
}

void mible_api_inner_msg_handle(T_IO_MSG *pmsg)
{
    switch (pmsg->type)
    {
    case MIBLE_API_MSG_TYPE_TIMEOUT:
        mible_handle_timeout(pmsg->u.buf);
        break;
#if MIBLE_MESH_API
    case MIBLE_API_MSG_TYPE_ADV_TIMEOUT:
        mible_gap_adv_send();
        break;
#endif
    default:
        break;
    }
}

bool mible_api_inner_msg_send(T_IO_MSG *pmsg)
{
    /* send event to notify upper layer task */
    if (!os_msg_send(mible_api_event_queue, &mible_api_event, 0))
    {
        MI_LOG_ERROR("failed to send msg to mible api event queue");
        return false;
    }

    if (!os_msg_send(mible_api_io_queue, pmsg, 0))
    {
        MI_LOG_ERROR("failed to send msg to mible api msg queue");
        return false;
    }

    return true;
}
