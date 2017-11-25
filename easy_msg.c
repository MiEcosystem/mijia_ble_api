/*****************************************************************************
File Name:    easy_msg.c
Discription:  消息池管理
History:
Date                Author                   Description
2017-02-18         Lucien                    Creat
****************************************************************************/
#include "easy_msg.h"
#include "mible_type.h"
#include "app.h"
#include "app_entry_point.h"
#include "easy_common.h"

#define APP_MSG_MAX_NUM                         (APP_MSG_SKY_LAST_MES - APP_MSG_CUS0 + 1)

typedef struct tag_msg_mng
{
		bool 				   active[APP_MSG_MAX_NUM];
}msg_mng_t;

struct msg_pack_t
{
		msg_handler_t handler;
		void *arg;
		uint8_t msg_pos;
};

static msg_mng_t s_msg_mng;

static void send_msg(msg_handler_t handler, void *arg,uint8_t msg_pos)
{
	// Instantiate the advertising update message to be sent
			struct msg_pack_t *msg = KE_MSG_ALLOC((APP_MSG_CUS0+msg_pos),
																																TASK_APP,
																																TASK_APP,
																																msg_pack_t);
			msg->handler = handler;
			msg->arg = arg;
			msg->msg_pos = msg_pos;
			// Send the message
			ke_msg_send(msg);
}


bool post_msg(msg_handler_t handler, void *arg)
{
		uint8_t msg_pos = 0xff;
		bool ret = false;
		disable_irqs();
		for(int i=0;i<APP_MSG_MAX_NUM;i++)
		{
				if(!s_msg_mng.active[i])
				{
						s_msg_mng.active[i] = true;
						msg_pos =  i;
						break;
				}
		}
		enable_irqs();
	
		if(msg_pos != 0xff)
		{
				send_msg(handler,arg,msg_pos);
				ret = true;
		}

		return ret;
}

static void recycle_msg(uint8_t msg_pos)
{
		disable_irqs();
		s_msg_mng.active[msg_pos] = false;
		enable_irqs();
}



enum process_event_response app_msg_api_process_handler(ke_msg_id_t const msgid,
                                                              void const *param,
                                                              ke_task_id_t const dest_id,
                                                              ke_task_id_t const src_id,
                                                              enum ke_msg_status_tag *msg_ret)
{
    if ((msgid < APP_MSG_CUS0) || (msgid > APP_MSG_SKY_LAST_MES))
    {
        return PR_EVENT_UNHANDLED;
    }
    else
    {
        struct msg_pack_t *msg = (struct msg_pack_t *)param;
				//执行回调函数
				if(msg->handler != 	NULL)
						msg->handler(msg->arg);
				recycle_msg(msg->msg_pos);
				
        *msg_ret = KE_MSG_CONSUMED;
        return PR_EVENT_HANDLED;
    }
}









