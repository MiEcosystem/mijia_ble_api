/*****************************************************************************
File Name:    user_default_handlers.c
Discription:  
History:
Date                Author                   Description
2018-02-24         Lucien                    Creat
****************************************************************************/
#include "user_default_handlers.h"
#include "app_easy_security.h"
#include "easy_timer.h"
#include "easy_msg.h"
#include "app_entry_point.h"

#if (BLE_MIJIA_SERVER)
#include "app_mijia.h"
#include "mible_type.h"
#include "mijia.h"
#include "mible_api.h"
#endif



void user_app_on_connection(uint8_t conidx, struct gapc_connection_req_ind const *param)
{
    if (app_env[conidx].conidx != GAP_INVALID_CONIDX)
    {
        if (user_default_hnd_conf.adv_scenario == DEF_ADV_WITH_TIMEOUT)
        {
            app_easy_gap_advertise_with_timeout_stop();
        }

        // Enable the created profiles/services
        app_prf_enable(conidx);
        
        if ((user_default_hnd_conf.security_request_scenario == DEF_SEC_REQ_ON_CONNECT) && (BLE_APP_SEC))
        {
            app_easy_security_request(conidx);
        }
#if (BLE_MIJIA_SERVER)
				mible_gap_evt_t evt;
				evt = MIBLE_GAP_EVT_CONNECTED;
				mible_gap_evt_param_t mi_param;
				memset(&mi_param,0,sizeof(mi_param));
				mi_param.conn_handle = conidx;
				mi_param.connect.conn_param.max_conn_interval = param->con_interval;
				mi_param.connect.conn_param.min_conn_interval = param->con_interval;
				mi_param.connect.conn_param.slave_latency = param->con_latency;
				mi_param.connect.conn_param.conn_sup_timeout = param->sup_to;
				memcpy(mi_param.connect.peer_addr,param->peer_addr.addr,BD_ADDR_LEN);
				mi_param.connect.role = MIBLE_GAP_PERIPHERAL;
		
				mible_gap_event_callback(evt,&mi_param);
#endif

    }
    else
    {
       // No connection has been established, restart advertising
       //CALLBACK_ARGS_0(user_default_app_operations.default_operation_adv)
    }
}


void app_on_update_params_rejected(const uint8_t handle)
{
#if (BLE_MIJIA_SERVER)

				mible_gap_evt_t evt;
				evt = MIBLE_GAP_EVT_CONN_PARAM_UPDATED;
				mible_gap_evt_param_t param;
				memset(&param,0,sizeof(param));
				//param.update_conn.conn_param
		
				mible_gap_event_callback(evt,&param);
#endif

}

//用户消息截取函数
void user_process_catch(ke_msg_id_t const msgid, void const *param,
                             ke_task_id_t const dest_id, ke_task_id_t const src_id) 
{
	#if (BLE_MIJIA_SERVER)
		enum ke_msg_status_tag  msg_ret;
		if(easy_timer_api_process_handler(msgid, param, dest_id, src_id,&msg_ret) == PR_EVENT_HANDLED) {
        return;
    }

		if(app_msg_api_process_handler(msgid, param, dest_id, src_id, &msg_ret) == PR_EVENT_HANDLED) {
        return;
    }
	#endif
}



void user_app_on_db_init_complete( void )
{
	#if (BLE_MIJIA_SERVER)
		mible_arch_event_t evt = MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP;
		mible_arch_evt_param_t mi_param;
		mi_param.srv_init_cmp.status = MI_SUCCESS;
		mi_param.srv_init_cmp.p_gatts_db = get_server_db();
		mible_arch_event_callback(evt,&mi_param);
	#endif
}








