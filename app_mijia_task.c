/**
 ****************************************************************************************
 *
 * @file app_mijia_task.c
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
#include "app_mijia_task.h"
#include "app_mijia.h"
#include "mijia_task.h"
#include "app_task.h"           // Application Task API
#include "app_entry_point.h"
#include "app.h"
#include "arch_console.h"
#if (BLE_MIJIA_SERVER)
#include "mible_api.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles sending indication completion conformation from mijia profile
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_mijia_send_indication_cfm_handler(ke_msg_id_t const msgid,
                                  struct mijia_indication_cfm const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
		arch_printf("state:%d\n",param->status);
		if(param->status == 0)
		{
				mible_gatts_evt_t evt = MIBLE_GATTS_EVT_IND_CONFIRM;
				mible_gatts_event_callback(evt,NULL);
		}
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disable indication from the MIJIA Server profile.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_mijia_write_val_ind_handler(ke_msg_id_t const msgid,
                                          struct mijia_val_write_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
		arch_printf("write_val handle:%d\n",param->handle);
		mible_gatts_evt_t evt = param->evt;
		mible_gatts_evt_param_t mi_param;
		mi_param.conn_handle = param->conhdl;
		mi_param.write.len = param->length;
		mi_param.write.data = (uint8_t*)param->value;
		mi_param.write.value_handle = param->value_handle;

		mible_gatts_event_callback(evt,&mi_param);

		//允许改变相应的值
		if(mi_param.write.permit != 0)
		{
				
		}
		return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief Handles sending data to bonded mijia server
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_mijia_send_data_handler(ke_msg_id_t const msgid,
                                  struct mijia_send_data_req const *req,
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
 * @brief Handles enable indication request from mijia profile
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int app_mijia_enable_ind_handler(ke_msg_id_t const msgid,
                                  struct mijia_enable_indication_req const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
		mible_gatts_evt_t evt = MIBLE_GATTS_EVT_WRITE;
		mible_gatts_evt_param_t mi_param;
		mi_param.conn_handle = param->conhdl;
		mi_param.write.len = sizeof(param->isEnable);
		mi_param.write.data = (uint8_t*)&(param->isEnable);
		mi_param.write.value_handle = param->value_handle;
		mible_gatts_event_callback(evt,&mi_param);
    return (KE_MSG_CONSUMED);
}



/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static const struct ke_msg_handler app_mijia_process_handlers[] =
{
    {MIJIA_SEND_INDICATION_CFM,         (ke_msg_func_t)app_mijia_send_indication_cfm_handler},
    {MIJIA_VAL_WRITE_IND,               (ke_msg_func_t)app_mijia_write_val_ind_handler},
		{MIJIA_ENABLE_IND_NTF_REQ,          (ke_msg_func_t)app_mijia_enable_ind_handler},
    {MIJIA_SEND_DATA_TO_MASTER,         (ke_msg_func_t)app_mijia_send_data_handler},
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_mijia_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret)
{

    return app_std_process_event(msgid, param, src_id, dest_id, msg_ret, app_mijia_process_handlers,
                                         sizeof(app_mijia_process_handlers) / sizeof(struct ke_msg_handler));
}
#endif //BLE_MIJIA_SERVER

#endif // BLE_CUSTOM_SERVER

#endif // BLE_APP_PRESENT

/// @} APPTASK
