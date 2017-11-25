/**
 ****************************************************************************************
 *
 * @file app_mijia_task.h
 *
 * @brief Custom Service task header.
 *
 * Copyright (C) 2015 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _APP_MIJIA_TASK_H_
#define _APP_MIJIA_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if BLE_CUSTOM_SERVER

#include "ke_msg.h"
#include "mijia_task.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if BLE_MIJIA_SERVER
/**
 ****************************************************************************************
 * @brief Process handler for the Mijia Service messages.
 * @param[in] msgid   Id of the message received
 * @param[in] param   Pointer to the parameters of the message
 * @param[in] dest_id ID of the receiving task instance (probably unused)
 * @param[in] src_id  ID of the sending task instance
 * @param[in] msg_ret Result of the message handler
 * @return Returns if the message is handled by the process handler
 ****************************************************************************************
 */
enum process_event_response app_mijia_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret);

/**
 ****************************************************************************************
 * @brief Other Process handlers for the Mijia Service messages.
 *
 ****************************************************************************************
 */

static int app_mijia_write_val_ind_handler(ke_msg_id_t const msgid,
                                          struct mijia_val_write_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id);

int app_mijia_send_data_handler(ke_msg_id_t const msgid,
                                  struct mijia_send_data_req const *req,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id);

int app_mijia_enable_ind_handler(ke_msg_id_t const msgid,
                                  struct mijia_enable_indication_req const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id);

int app_mijia_send_indication_cfm_handler(ke_msg_id_t const msgid,
									  struct mijia_indication_cfm const *param,
									  ke_task_id_t const dest_id,
									  ke_task_id_t const src_id);



#endif // BLE_MIJIA_SERVER

#endif // BLE_CUSTOM_SERVER

#endif // _APP_MIJIA_TASK_H_
