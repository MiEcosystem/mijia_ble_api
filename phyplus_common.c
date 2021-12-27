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
#include "mible_api.h"
#define MI_LOG_MODULE_NAME "RTK_COMMON"
#include "mible_log.h"
//#include "platform_misc.h"

static void *mible_api_event_queue;
static void *mible_api_io_queue;

static uint8_t mible_api_event;

extern void mible_handle_timeout(void *timer);
#if MIBLE_MESH_API
extern mible_status_t mible_gap_adv_send(void);
#endif

void mible_api_init(uint8_t evt_type, void *pevent_queue)
{
}

void mible_api_inner_msg_handle(uint8_t event)
{

}


int mible_log_printf(const char * sFormat, ...)
{

    
    return MI_SUCCESS;
}

int mible_log_hexdump(void* array_base, uint16_t array_size)
{

    return MI_SUCCESS;
}

/**
 *@brief    reboot device.
 *@return   0: success, negetive value: failure
 */
mible_status_t mible_reboot(void)
{
    return MI_SUCCESS;
}

/**
 *@brief    set node tx power.
 *@param    [in] power : TX power in 0.1 dBm steps.
 *@return   0: success, negetive value: failure
 */
mible_status_t mible_set_tx_power(int16_t power)
{

    return MI_SUCCESS;
}


mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
}

void mible_tasks_exec(void)
{
}

void mi_task_start(uint8_t event_mi, void *pevent_queue)
{
}
