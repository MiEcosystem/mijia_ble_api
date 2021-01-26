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
#ifndef _RTK_COMMON_H_
#define _RTK_COMMON_H_

#include <app_msg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIBLE_API_MSG_NUM                           20
#define MIBLE_MAX_TASK_NUM                          10

/* mible api inner message type */
#define MIBLE_API_MSG_TYPE_TIMEOUT                  0x00
#define MIBLE_API_MSG_TYPE_ADV_TIMEOUT              0x01

void mible_api_init(uint8_t evt_type, void *pevent_queue);
bool mible_api_inner_msg_send(T_IO_MSG *pmsg);
void mible_api_inner_msg_handle(uint8_t event);
void mi_task_start(uint8_t event_mi, void *pevent_queue);

#ifdef __cplusplus
}
#endif

#endif /* _RTK_COMMON_H_ */
