/**
 ****************************************************************************************
 *
 * @file app_sps_task.c
 *
 * @brief Custom Service application Task Implementation.
 *
 * Copyright (C) 2012 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"              // SW configuration

#if (BLE_APP_PRESENT)

#if (BLE_CUSTOM_SERVER)
#include "app_sps_task.h"
#include "app_sps.h"
#include "sps_task.h"
#include "app_task.h"           // Application Task API
#include "app_entry_point.h"
#include "app.h"
#include "mible_log.h"

#if (BLE_SPS_SERVER)
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles sending indication completion conformation from sps profile
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_sps_send_indication_cfm_handler(ke_msg_id_t const msgid,
                                  struct sps_indication_cfm const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
		MI_LOG_DEBUG("send cfm status:%d\n", param->status);
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disable indication from the SPS Server profile.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_sps_write_val_ind_handler(ke_msg_id_t const msgid,
                                          struct sps_val_write_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
	  ble_sps_process_received_data((uint8_t *)(param->value), param->length); 
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles sending data to bonded sps server
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_sps_send_data_handler(ke_msg_id_t const msgid,
                                  struct sps_send_data_req const *req,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
	#ifdef CATCH_LOG
		arch_printf("\r\n button 1");
	#endif

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles enable indication request from sps profile
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_sps_enable_ind_handler(ke_msg_id_t const msgid,
                                  struct sps_enable_indication_req const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
		MI_LOG_DEBUG("is_enable:%d\n",param->isEnable);

    return (KE_MSG_CONSUMED);
}



/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static const struct ke_msg_handler app_sps_process_handlers[] =
{
    {SPS_SEND_INDICATION_CFM,         (ke_msg_func_t)app_sps_send_indication_cfm_handler},
    {SPS_VAL_WRITE_IND,               (ke_msg_func_t)app_sps_write_val_ind_handler},
    {SPS_ENABLE_IND_REQ,              (ke_msg_func_t)app_sps_enable_ind_handler},
    {SPS_SEND_DATA_TO_MASTER,         (ke_msg_func_t)app_sps_send_data_handler},
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_sps_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret)
{

    return app_std_process_event(msgid, param, src_id, dest_id, msg_ret, app_sps_process_handlers,
                                         sizeof(app_sps_process_handlers) / sizeof(struct ke_msg_handler));
}
#endif //BLE_SPS_SERVER

#endif // BLE_CUSTOM_SERVER

#endif // BLE_APP_PRESENT

/// @} APPTASK
