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
#ifndef _RTK_MESH_H_
#define _RTK_MESH_H_

#include <app_msg.h>

/* the number of mijia mesh inner message */
#define MI_INNER_MSG_NUM                         16

void mi_mesh_start(uint8_t event_mi, void *event_queue);
void mi_inner_msg_handle(uint8_t event);
void mi_handle_gap_msg(T_IO_MSG *pmsg);
void mi_startup_delay(void);

void get_device_id(uint8_t *pdid);

void mi_gatts_init(void);
void mi_gatts_suspend(void);
void mi_gatts_resume(void);

#endif /* _RTK_MESH_H_ */
