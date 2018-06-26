/**
 ****************************************************************************************
 *
 * @file app_sps.c
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
#include "app_sps.h"
#include "sps_task.h"
#include "attm_db.h"
#include "attm_db_128.h"
#include "gapc.h"
#include "prf_types.h"
#include "app_prf_types.h"
#include "app_prf_perm_types.h"
#include "user_custs_config.h"
#include "prf_utils.h"
#include "mible_log.h"



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */


#if (BLE_SPS_SERVER)




/**
 ****************************************************************************************
 * @brief Initialize sps application.
 * @return void
 ****************************************************************************************
 */
void app_sps_init(void)
{

}

/**
 ****************************************************************************************
 * @brief Create sps profile database.
 * @return void
 ****************************************************************************************
 */
void app_sps_create_db(void)
{
    struct sps_db_cfg *db_cfg;

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             sizeof(struct sps_db_cfg));

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = get_user_prf_srv_perm(TASK_ID_SPS);
    req->prf_task_id = TASK_ID_SPS;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    // Set parameters
    db_cfg = (struct sps_db_cfg *) req->param;
    // Attribute table. In case the handle offset needs to be saved
    db_cfg->att_tbl = NULL;
    db_cfg->cfg_flag = 0;
    db_cfg->features = 0;

    // Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief Other sps app funcs. 
 * 
 ****************************************************************************************
 */


void ble_sps_process_received_data(uint8_t* pData, uint32_t length)
{
		MI_LOG_DEBUG("rec:");
		MI_LOG_HEXDUMP(pData,length);
}

void ble_sps_send_data(uint8_t* pData,uint8_t length)
{

		if(length < 20){
				struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
				ke_task_id_t temp_dst = prf_dst_task_get(&(sps_env->prf_env), sps_env->cursor);
				ke_task_id_t temp_src = prf_src_task_get(&(sps_env->prf_env), sps_env->cursor);
				struct sps_indication_req *req = KE_MSG_ALLOC(SPS_SEND_REQ,
		                                                  temp_src, 
		                                                  temp_dst,
		                                                  sps_indication_req);
		    req->conhdl = 0;//active_conhdl;
		    memcpy(req->value, pData, length);
		    req->length = length;

		    ke_msg_send(req);
		}
}



#endif // (BLE_SPS_SERVER)

#endif // (BLE_CUSTOM_SERVER)
