/*****************************************************************************
File Name:    easy_timer.h
Discription:  定时器管理
History:
Date                Author                   Description
2017-11-11         Lucien                    Creat
****************************************************************************/
#ifndef __EASY_TIMER_H__
#define __EASY_TIMER_H__

#include "mible_type.h"




typedef void (*easy_timer_handler)(void*);

void init_easy_timer(void);


bool easy_timer_create(void** p_timer_id,
    mible_timer_handler timeout_handler,
    mible_timer_mode mode);


bool easy_timer_delete(void* timer_id);


bool easy_timer_start(void* timer_id, uint32_t timeout_value,
    void* p_context);


bool easy_timer_stop(void* timer_id);


enum process_event_response easy_timer_api_process_handler(ke_msg_id_t const msgid,
                                                          void const *param,
                                                          ke_task_id_t const dest_id,
                                                          ke_task_id_t const src_id,
                                                          enum ke_msg_status_tag *msg_ret);





#endif
