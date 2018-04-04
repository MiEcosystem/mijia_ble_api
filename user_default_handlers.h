/*****************************************************************************
File Name:    user_default_handlers.h
Discription:  
History:
Date                Author                   Description
2018-02-24         Lucien                    Creat
****************************************************************************/
#ifndef __USER_DEFAULT_HANDLERS_H__
#define __USER_DEFAULT_HANDLERS_H__

#include "gapc_task.h"







void user_app_on_connection(uint8_t conidx, struct gapc_connection_req_ind const *param);


void app_on_update_params_rejected(const uint8_t handle);


void user_process_catch(ke_msg_id_t const msgid, void const *param,
                             ke_task_id_t const dest_id, ke_task_id_t const src_id);


void user_app_on_db_init_complete(void);





#endif
