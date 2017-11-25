/*****************************************************************************
File Name:    easy_msg.h
Discription:  消息池管理
History:
Date                Author                   Description
2017-11-10         Lucien                    Creat
****************************************************************************/
#ifndef __EASY_MSG_H__
#define __EASY_MSG_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>




typedef void (*msg_handler_t) (void* arg);





bool post_msg(msg_handler_t handler, void *arg);






enum process_event_response app_msg_api_process_handler(ke_msg_id_t const msgid,
                                                              void const *param,
                                                              ke_task_id_t const dest_id,
                                                              ke_task_id_t const src_id,
                                                              enum ke_msg_status_tag *msg_ret);






#endif
