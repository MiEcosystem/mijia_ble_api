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

static uint8_t mible_api_event;

extern void mible_handle_timeout(void *timer);
#if MIBLE_MESH_API
extern mible_status_t mible_gap_adv_send(void);
#endif

void mible_api_init(uint8_t evt_type, void *pevent_queue)
{
    mible_api_event = evt_type;
    mible_api_event_queue = pevent_queue;
    os_msg_queue_create(&mible_api_io_queue, MIBLE_API_MSG_NUM, sizeof(T_IO_MSG));
}

void mible_api_inner_msg_handle(uint8_t event)
{
    if (event != mible_api_event)
    {
        MI_LOG_ERROR("mible_api_inner_msg_handle: fail, event(%d) is not mible api event(%d)!", event, mible_api_event);
        return;
    }
    
    T_IO_MSG io_msg;
    if (os_msg_recv(mible_api_io_queue, &io_msg, 0) == false)
    {
        MI_LOG_ERROR("mible_api_inner_msg_handle: fail to receive msg!");
        return;
    }
                
    switch (io_msg.type)
    {
    case MIBLE_API_MSG_TYPE_TIMEOUT:
        mible_handle_timeout(io_msg.u.buf);
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
    if (!os_msg_send(mible_api_io_queue, pmsg, 0))
    {
        MI_LOG_ERROR("failed to send msg to mible api msg queue");
        return false;
    }
    
    if (!os_msg_send(mible_api_event_queue, (void*)&mible_api_event, 0))
    {
        MI_LOG_ERROR("failed to send msg to mible api event queue");
        return false;
    }

    return true;
}
