/**
 ****************************************************************************************
 *
 * @file app_mijia.c
 *
 * @brief Custom profiles application file.
 *
 * Copyright (C) 2015-2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "rwble_config.h"              // SW configuration
#if (BLE_CUSTOM_SERVER)
#include "app_mijia.h"
#include "mijia_task.h"
#include "attm_db.h"
#include "attm_db_128.h"
#include "gapc.h"
#include "prf_types.h"
#include "app_prf_types.h"
#include "app_prf_perm_types.h"
#include "user_custs_config.h"
#include "prf_utils.h"
#include "arch_console.h"
#include "easy_nvds.h"
#include "app_task.h"
#include "easy_timer.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */


#if (BLE_MIJIA_SERVER)


/**
 ****************************************************************************************
 * @brief Initialize mijia application.
 * @return void
 ****************************************************************************************
 */
void app_mijia_init(void)
{
    easy_nvds_init();
		init_easy_timer();
		simulation_miserver_test();
		
}

/**
 ****************************************************************************************
 * @brief Create mijia profile database.
 * @return void
 ****************************************************************************************
 */
void app_mijia_create_db(void)
{
    struct mijia_db_cfg *db_cfg;

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             sizeof(struct mijia_db_cfg));

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = get_user_prf_srv_perm(TASK_ID_MIJIA);
    req->prf_task_id = TASK_ID_MIJIA;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    // Set parameters
    db_cfg = (struct mijia_db_cfg *) req->param;
    // Attribute table. In case the handle offset needs to be saved
    db_cfg->att_tbl = NULL;
    db_cfg->cfg_flag = 0;
    db_cfg->features = 0;

    // Send the message
    ke_msg_send(req);
}


mible_status_t ble_mijia_send_data(uint16_t srv_handle,uint16_t value_handle,uint8_t* pData, uint32_t length)
{
		mible_status_t ret = MI_SUCCESS;
		struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
		uint8_t state = ke_state_get(TASK_APP);

    // Check if we are not already in a connected state
    if (state == APP_CONNECTED)
    {
				ke_task_id_t temp_dst = prf_dst_task_get(&(mijia_env->prf_env), mijia_env->cursor);
				ke_task_id_t temp_src = prf_src_task_get(&(mijia_env->prf_env), mijia_env->cursor);

				struct mijia_notifcation_req req1;
				req1.length = length;
				req1.value_handle  = value_handle;
				memcpy(&req1.value, pData, length);
				req1.conhdl = 0;
					
				ret = mijia_send_notifcation_req_handler_direct(MIJIA_SEND_NOTIFCATION_REQ,&req1,temp_dst,temp_src);
				COMPrintf("ret:%d\n",ret);
		}
		else
				ret = MI_ERR_INVALID_STATE;
		
		return ret;
}




#endif // (BLE_MIJIA_SERVER)

#endif // (BLE_CUSTOM_SERVER)
