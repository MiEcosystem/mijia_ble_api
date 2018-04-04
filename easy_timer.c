/*****************************************************************************
File Name:    easy_timer.c
Discription:  定时器管理
History:
Date                Author                   Description
2017-11-11         Lucien                    Creat
****************************************************************************/
#include "easy_timer.h"
#include "app.h"
#include "app_entry_point.h"
#include "easy_msg.h"
#include "easy_common.h"
#include "app_easy_msg_utils.h"
#include "mijia_task.h"
#if (BLE_MIJIA_SERVER)
#define TIMER_MAX_NUM                         (APP_TIMER_CUS_LAST_MES - APP_TIMER_CUS_MES0 + 1)
#define ILLG_TIMER_ID					0

#define APP_TIMER_HND_IS_VALID(timer_id)     ((timer_id > 0) && (timer_id <= TIMER_MAX_NUM))
#define APP_TIMER_HND_TO_MSG_ID(timer_id)    (timer_id - 1 + APP_TIMER_CUS_MES0)
#define APP_TIMER_MSG_ID_TO_HND(timer_msg)   (timer_msg - APP_TIMER_CUS_MES0 + 1)


typedef struct tag_timer_mng
{
		bool									used[TIMER_MAX_NUM];
		bool 				   				active[TIMER_MAX_NUM];
		mible_timer_handler		handler[TIMER_MAX_NUM];
		mible_timer_mode      mode[TIMER_MAX_NUM];
		void* 								context[TIMER_MAX_NUM];
		uint32_t							time_out[TIMER_MAX_NUM];
		uint32_t							time_id_buf[TIMER_MAX_NUM];
}timer_mng_t;


struct create_timer_t
{
    uint32_t timer_id;
    uint16_t delay;
};


static timer_mng_t s_timer_mng;

/*
 * @brief 	Delete a timer.
 * @param 	[out] timer_id: timer id
 *
 * @return  true             If the timer was create;
 *          false            create . failed
 * @note 	
 * */
bool easy_timer_create(void** p_timer_id,
    mible_timer_handler timeout_handler,
    mible_timer_mode mode)
{
		uint32_t timer_id = ILLG_TIMER_ID;
		bool ret = false;
		disable_irqs();
		for(int i=0;i<TIMER_MAX_NUM;i++)
		{
				if(!s_timer_mng.used[i])
				{
						s_timer_mng.used[i] = true;
						s_timer_mng.handler[i] = timeout_handler;
						s_timer_mng.mode[i]  = mode;
						timer_id =  i+1;
						break;
				}
		}
		enable_irqs();

		if(timer_id != ILLG_TIMER_ID)
		{
				ret = true;
				*((uint32_t **)p_timer_id) = &(s_timer_mng.time_id_buf[timer_id-1]);
				s_timer_mng.time_id_buf[timer_id-1] = timer_id;
		}
		else
				p_timer_id = NULL;

		return ret;
}




static void recycle_timer(uint32_t timer_id)
{
		disable_irqs();
		if(APP_TIMER_HND_IS_VALID(timer_id))
		{
			s_timer_mng.handler[timer_id-1] = NULL;
			s_timer_mng.used[timer_id-1] = false;
			s_timer_mng.active[timer_id-1] = false;
			s_timer_mng.mode[timer_id-1] = MIBLE_TIMER_SINGLE_SHOT;
			s_timer_mng.time_id_buf[timer_id-1] = timer_id;
		}
		enable_irqs();
}

void init_easy_timer(void)
{
		for(int i=0;i<TIMER_MAX_NUM;i++)
			recycle_timer(i+1);

}

/*
 * @brief 	Delete a timer.
 * @param 	[in] timer_id: timer id
 *
 * @return  true             If the timer was delated;
 *          false            timer id INVALID.
 * @note 	
 * */
bool easy_timer_delete(void* timer_id) 
{ 
	bool ret = false;
	if(timer_id != NULL)
	{
				uint32_t id = *((uint32_t*)timer_id);
				uint32_t msg_id = APP_TIMER_HND_TO_MSG_ID(id);
				ke_timer_clear(msg_id,TASK_APP);
				recycle_timer(id);	
				ret = true;
	}
	
	return ret; 
}



bool easy_timer_start(void* timer_id, uint32_t timeout_value,
    void* p_context)
{
		uint32_t id = *((uint32_t*)(timer_id));
		uint32_t timeout = ((timeout_value/10) + (timeout_value%10==0 ? 0:1) );
		uint32_t msg_id = APP_TIMER_HND_TO_MSG_ID(id);
		
		if (app_check_BLE_active())
    {
				if(APP_TIMER_HND_IS_VALID(id))
				{
						s_timer_mng.active[id-1] = true;
						s_timer_mng.time_out[id-1] = timeout;
						s_timer_mng.context[id-1] = p_context;
						ke_timer_clear(msg_id,TASK_APP);
						ke_timer_set(msg_id,TASK_APP,timeout);
						return true;
				}
    }
    else
    {
    		ke_timer_clear(msg_id,TASK_APP);
        arch_ble_force_wakeup(); //wake_up BLE
        //send a message to wait for BLE to be woken up before executing the
        struct create_timer_t *req = KE_MSG_ALLOC(APP_CREATE_TIMER, TASK_APP, TASK_APP,
                                                       create_timer_t);

        req->delay = timeout;
        req->timer_id = id;
        ke_msg_send(req);
    }
		
    return false;
}

/*
 * @brief 	Stop a timer.
 * @param 	[in] timer_id: timer id
 * @return  true             If the timer was successfully stopped.
 *          false        Invalid timer id supplied.
 *
 * */
bool easy_timer_stop(void* timer_id) 
{ 
  uint32_t id = *((uint32_t*)(timer_id));
	if(APP_TIMER_HND_IS_VALID(id))
	{
			uint32_t msg_id = APP_TIMER_HND_TO_MSG_ID(id);
			s_timer_mng.active[id-1] = false;
			ke_timer_clear(msg_id,TASK_APP);
			return true;
	}
	return false; 
}

/**
 ****************************************************************************************
 * @brief The actual timer handler that calls the user callback.
 * @param[in] msgid Id of the message received
 * @param[in] param No parameters are required
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance
 * @return KE_MSG_CONSUMED
 ****************************************************************************************
 */
static int call_callback_handler(ke_msg_id_t const msgid,
                                 void const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    uint32_t timer_id = APP_TIMER_MSG_ID_TO_HND(msgid);
		if(APP_TIMER_HND_IS_VALID(timer_id))
		{
				mible_timer_handler handler = NULL;
				void* 							context = NULL;
				disable_irqs();
				handler = 	s_timer_mng.handler[timer_id-1];
				context = s_timer_mng.context[timer_id-1];
				enable_irqs();
				if(handler != NULL)
						handler(context);
				
				if(s_timer_mng.active[timer_id-1] == true &&\
					 s_timer_mng.mode[timer_id-1] == MIBLE_TIMER_REPEATED)
						ke_timer_set(msgid,TASK_APP,s_timer_mng.time_out[timer_id-1]);
				else
					  recycle_timer(timer_id);
		}

    return KE_MSG_CONSUMED;
}


/**
 ****************************************************************************************
 * @brief Handler function that is called when the TASK_APP receives the APP_CREATE_CUS_TIMER
 *        message. Called after the ble wakes up in the case where the ble is sleeping 
 *        when trying to set a new timer.
 * @param[in] msgid Id of the message received
 * @param[in] param The timer details to be set
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance
 * @return KE_MSG_CONSUMED
 ****************************************************************************************
 */
static int create_timer_handler(ke_msg_id_t const msgid,
                                struct create_timer_t const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    // Sanity checks
    ASSERT_ERROR(param->delay > 0);                  // Delay should not be zero
    ASSERT_ERROR(param->delay < KE_TIMER_DELAY_MAX); // Delay should not be more than maximum allowed

    ke_timer_set(APP_TIMER_HND_TO_MSG_ID(param->timer_id), TASK_APP, param->delay);
    return KE_MSG_CONSUMED;
}




enum process_event_response easy_timer_api_process_handler(ke_msg_id_t const msgid,
                                                          void const *param,
                                                          ke_task_id_t const dest_id,
                                                          ke_task_id_t const src_id,
                                                          enum ke_msg_status_tag *msg_ret)
{
    switch (msgid)
    {
				case APP_CREATE_CUS_TIMER:
							*msg_ret = (enum ke_msg_status_tag)create_timer_handler(msgid, param, dest_id, src_id);
						return PR_EVENT_HANDLED;
        default:
            if ((msgid < APP_TIMER_CUS_MES0) || (msgid > APP_TIMER_CUS_LAST_MES))
            {
                return PR_EVENT_UNHANDLED;
            }
            else
            {
                *msg_ret = (enum ke_msg_status_tag)call_callback_handler(msgid, param, dest_id, src_id);
            }
            return PR_EVENT_HANDLED;
    }
}


#endif





