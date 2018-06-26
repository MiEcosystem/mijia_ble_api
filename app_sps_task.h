/**
 ****************************************************************************************
 *
 * @file app_sps_task.h
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

#ifndef _APP_SPS_TASK_H_
#define _APP_SPS_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if BLE_CUSTOM_SERVER

#include "ke_msg.h"
#include "sps_task.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if BLE_SPS_SERVER
/**
 ****************************************************************************************
 * @brief Process handler for the Sps Service messages.
 * @param[in] msgid   Id of the message received
 * @param[in] param   Pointer to the parameters of the message
 * @param[in] dest_id ID of the receiving task instance (probably unused)
 * @param[in] src_id  ID of the sending task instance
 * @param[in] msg_ret Result of the message handler
 * @return Returns if the message is handled by the process handler
 ****************************************************************************************
 */
enum process_event_response app_sps_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret);

/**
 ****************************************************************************************
 * @brief Other Process handlers for the Sps Service messages.
 *
 ****************************************************************************************
 */

static int app_sps_write_val_ind_handler(ke_msg_id_t const msgid,
                                          struct sps_val_write_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id);

int app_sps_send_data_handler(ke_msg_id_t const msgid,
                                  struct sps_send_data_req const *req,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id);

int app_sps_enable_ind_handler(ke_msg_id_t const msgid,
                                  struct sps_enable_indication_req const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id);

int app_sps_send_indication_cfm_handler(ke_msg_id_t const msgid,
									  struct sps_indication_cfm const *param,
									  ke_task_id_t const dest_id,
									  ke_task_id_t const src_id);



#endif // BLE_SPS_SERVER

#endif // BLE_CUSTOM_SERVER

#endif // _APP_SPS_TASK_H_
