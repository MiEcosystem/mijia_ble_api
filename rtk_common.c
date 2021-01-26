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
#include <gap_adv.h>
#include <os_msg.h>
#include <os_task.h>
#include "mible_api.h"
#define MI_LOG_MODULE_NAME "RTK_COMMON"
#include "mible_log.h"
#include "platform_misc.h"

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

#ifndef LOG_IN_SEGGER_RTT
#include "platform_diagnose.h"
#include "trace.h"
#include <stdarg.h>
#include <stdio.h>
int mible_log_printf(const char * sFormat, ...)
{
    char buffer[256];
    va_list ParamList;
    
    va_start(ParamList, sFormat);
    vsprintf(buffer, sFormat, ParamList);
    log_direct(COMBINE_TRACE_INFO(TYPE_BEE2, SUBTYPE_DIRECT, 0, 0), (const char*)buffer);
    va_end(ParamList);
    
    return MI_SUCCESS;
}

int mible_log_hexdump(void* array_base, uint16_t array_size)
{
    do {\
        static const char format[] TRACE_DATA = "[hex][%d] %b\n";\
        log_buffer(COMBINE_TRACE_INFO(TYPE_BEE2, SUBTYPE_FORMAT, 0, 0), (uint32_t)format, 2, array_size, TRACE_BINARY(array_size, array_base));\
    } while (0);

    return MI_SUCCESS;
}
#else
#include "third_party/SEGGER_RTT/SEGGER_RTT.h"
#include <stdarg.h>
extern int SEGGER_RTT_vprintf(unsigned BufferIndex, const char * sFormat,
                                va_list * pParamList);
int mible_log_printf(const char * sFormat, ...)
{
    va_list ParamList;
    va_start(ParamList, sFormat);
    SEGGER_RTT_vprintf(0, sFormat, &ParamList);
    va_end(ParamList);

    return MI_SUCCESS;
}

int mible_log_hexdump(void* array_base, uint16_t array_size)
{
    do {                                                                           \
        for (int Mi_index = 0; Mi_index<(array_size); Mi_index++)                                       \
            SEGGER_RTT_printf(0, (Mi_index+1)%16?"%02X ":"%02X\n", ((char*)(array_base))[Mi_index]);\
        if (array_size%16) SEGGER_RTT_printf(0,"\n");                               \
    } while(0);
    return MI_SUCCESS;
}
#endif

/**
 *@brief    reboot device.
 *@return   0: success, negetive value: failure
 */
mible_status_t mible_reboot(void)
{
    plt_reset(0);
    return MI_SUCCESS;
}

/**
 *@brief    set node tx power.
 *@param    [in] power : TX power in 0.1 dBm steps.
 *@return   0: success, negetive value: failure
 */
mible_status_t mible_set_tx_power(int16_t power)
{
    uint8_t tx_gain;

    if(power <= -200){
        // 0x30      -20 dBm
        tx_gain = 0X30;
    }else if(power <= 0){
        // 0x80      0 dBm
        tx_gain = 0X80;
    }else if(power <= 30){
        // 0x90      3 dBm
        tx_gain = 0X90;
    }else if(power <= 40){
        // 0xA0      4 dBm
        tx_gain = 0XA0;
    }else{
        // 0xD0      7.5 dBm
        tx_gain = 0XD0;
    }
    le_adv_set_tx_power(GAP_ADV_TX_POW_SET_1M, tx_gain);
    MI_LOG_DEBUG("set tx power 0x%x.\n", tx_gain);

    return MI_SUCCESS;
}

/* TASK schedulor related function  */
typedef struct {
    mible_handler_t handler;
    void *p_ctx;
} mible_task_t;

static void *evt_queue_handle;
static void *mi_queue_handle;
static uint8_t mi_task_event;

mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
    mible_task_t event;
    event.handler = handler;
    event.p_ctx = arg;

    if (os_msg_send(mi_queue_handle, &event, 0))
    {
        uint8_t event_mi = mi_task_event;
        os_msg_send(evt_queue_handle, &event_mi, 0);
        return MI_SUCCESS;
    }
    else
    {
        return MI_ERR_INTERNAL;
    }
}

void mible_tasks_exec(void)
{
    mible_task_t event;

    if (os_msg_recv(mi_queue_handle, &event, 0) && event.handler != NULL)
    {
        event.handler(event.p_ctx);
    }
}

void mi_task_start(uint8_t event_mi, void *pevent_queue)
{
    mi_task_event = event_mi;
    evt_queue_handle = pevent_queue;
    os_msg_queue_create(&mi_queue_handle, MIBLE_MAX_TASK_NUM, sizeof(mible_task_t));
}
