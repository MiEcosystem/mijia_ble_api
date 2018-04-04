/**
 ****************************************************************************************
 *
 * @file mijia_task.c
 *
 * @brief Custom Service profile task source file.
 *
 * Copyright (C) 2014 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include "rwble_config.h"              // SW configuration

#if (BLE_MIJIA_SERVER)
#include "mijia_task.h"
#include "mijia.h"
#include "custom_common.h"
#include "app_customs.h"
#include "attm_db_128.h"
#include "ke_task.h"
#include "gapc.h"
#include "gapc_task.h"
#include "gattc_task.h"
#include "attm_db.h"
#include "prf_utils.h"
#include "app_prf_types.h"
#include "arch_console.h"
#include "mible_api.h"
#include "mible_port.h"
#include "mible_log.h"
#include "mi_app_customs.h"

#if (BLE_CUSTOM_SERVER)
#include "user_custs_config.h"
#endif // (BLE_CUSTOM_SERVER)


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Stores characteristic value.
 * @param[in] att_idx  Custom attribut index.
 * @param[in] length   Value length.
 * @param[in] data     Pointer to value data.
 * @return 0 on success.
 ****************************************************************************************
 */
static int mijia_att_set_value(uint8_t att_idx, uint16_t length, const uint8_t *data)
{
    struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    // Check value in already present in service
    struct mijia_val_elmt *val = (struct mijia_val_elmt *) co_list_pick(&(mijia_env->values));
    // loop until value found
    while (val != NULL)
    {
        // value is present in service
        if (val->att_idx == att_idx)
        {
            // Value present but size changed, free old value
            if (length != val->length)
            {
                co_list_extract(&mijia_env->values, &val->hdr, 0);
                ke_free(val);
                val = NULL;
            }
            break;
        }

        val = (struct mijia_val_elmt *)val->hdr.next;
    }

    if (val == NULL)
    {
        // allocate value data
        val = (struct mijia_val_elmt *) ke_malloc(sizeof(struct mijia_val_elmt) + length, KE_MEM_ATT_DB);
        // insert value into the list
        co_list_push_back(&mijia_env->values, &val->hdr);
    }
    val->att_idx = att_idx;
    val->length = length;
    memcpy(val->data, data, length);

    return 0;
}

/**
 ****************************************************************************************
 * @brief Read characteristic value from.
 * Function checks if attribute exists, and if so return its length and pointer to data.
 * @param[in]  att_idx  Custom attribute index.
 * @param[out] length   Pointer to variable that receive length of the attribute.
 * @param[out] data     Pointer to variable that receive pointer characteristic value.
 * @return 0 on success, ATT_ERR_ATTRIBUTE_NOT_FOUND if there is no value for such attribyte.
 ****************************************************************************************
 */
static int mijia_att_get_value(uint8_t att_idx, uint16_t *length, const uint8_t **data)
{
    struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    // Check value in already present in service
    struct mijia_val_elmt *val = (struct mijia_val_elmt *) co_list_pick(&mijia_env->values);
    ASSERT_ERR(data);
    ASSERT_ERR(length);

    // loop until value found
    while (val != NULL)
    {
        // value is present in service
        if (val->att_idx == att_idx)
        {
            *length = val->length;
            *data = val->data;
            break;
        }

        val = (struct mijia_val_elmt *)val->hdr.next;
    }

    if (val == NULL)
    {
        *length = 0;
        *data = NULL;
    }
    return val ? 0 : ATT_ERR_ATTRIBUTE_NOT_FOUND;
}

/**
 ****************************************************************************************
 * @brief Sets initial values for all Clinet Characteristic Configurations.
 * @param[in]  att_db     Custom service attribute definition table.
 * @param[in]  max_nb_att Number of elements in att_db.
 ****************************************************************************************
 */
void mijia_init_ccc_values(const struct attm_desc_128 *att_db, int max_nb_att)
{
    // Default values 0 means no notification
    uint8_t ccc_values[BLE_CONNECTION_MAX] = {0};
    int i;

    // Start form 1, skip service description
    for (i = 1; i < max_nb_att; i++)
    {
        // Find only CCC characteristics
        if (att_db[i].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)att_db[i].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            // Set default values for all possible connections
            mijia_att_set_value(i, sizeof(ccc_values), ccc_values);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Set value of CCC for given attribute and connection index.
 * @param[in] conidx   Connection index.
 * @param[in] att_idx  CCC attribute index.
 * @param[in] cc       Value to store.
 ****************************************************************************************
 */
void mijia_set_ccc_value(uint8_t conidx, uint8_t att_idx, uint16_t ccc)
{
    uint16_t length;
    const uint8_t *value;
    uint8_t new_value[BLE_CONNECTION_MAX];
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    mijia_att_get_value(att_idx, &length, &value);
    ASSERT_ERR(length);
    ASSERT_ERR(value);
    memcpy(new_value, value, length);
    // For now there are only two valid values for ccc, store just one byte other is 0 anyway
    new_value[conidx] = (uint8_t)ccc;
    mijia_att_set_value(att_idx, length, new_value);
}

void ble_mijia_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
    uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
    uint8_t len, uint8_t type)
{
		uint8_t att_idx = 0;
		uint16_t ccc = 0;
		ccc = type;
		struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
		
		uint8_t status = mijia_get_att_idx(mijia_env->shdl+char_value_handle, &att_idx);
		mijia_set_ccc_value(conn_handle,att_idx,ccc);
}


bool ble_mijia_gatts_value_get(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len)
{
	  uint16_t length;
    uint8_t *value;

		struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);

		uint8_t state = attmdb_get_value(mijia_env->shdl+value_handle,&length,&value);
		if(state == ATT_ERR_NO_ERROR)
		{
				*p_len = length;
				memcpy(p_value,value,length);
				return false;
		}
		else
				return true;
}

/**
 ****************************************************************************************
 * @brief Read value of CCC for given attribute and connection index.
 * @param[in]  conidx   Connection index.
 * @param[in]  att_idx  Custom attribute index.
 * @return Value of CCC.
 ****************************************************************************************
 */
static uint16_t mijia_get_ccc_value(uint8_t conidx, uint8_t att_idx)
{
    uint16_t length;
    const uint8_t *value;
    uint16_t ccc_value;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    mijia_att_get_value(att_idx, &length, &value);
    ASSERT_ERR(length);
    ASSERT_ERR(value);

    ccc_value = value[conidx];

    return ccc_value;
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * @details The GATTC_CMP_EVT message that signals the completion of a GATTC_NOTIFY
 *          operation is sent back as soon as the notification PDU has been sent over
 *          the air.
 *          The GATTC_CMP_EVT message that signals the completion of a GATTC_INDICATE
 *          operation is sent back as soon as the ATT_HANDLE_VALUE_CONFIRMATION PDU is
 *          received confirming that the indication has been correctly received by the
 *          peer device.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);

    if (param->operation == GATTC_INDICATE || param->operation == GATTC_NOTIFY)
    {
        mijia_ind_ntf_cfm_send(param->status);
    }

    return (KE_MSG_CONSUMED);

}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_REQ_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
		arch_printf("read_req\n");
    struct gattc_read_cfm * cfm;
    uint8_t att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = mijia_get_att_idx(param->handle, &att_idx);
    uint16_t length = 0;
    uint16_t ccc_val = 0;
    struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);

    // If the attribute has been found, status is GAP_ERR_NO_ERROR
    if (status == GAP_ERR_NO_ERROR)
    {
        const struct cust_prf_func_callbacks *callbacks = custs_get_func_callbacks(TASK_ID_MIJIA);

        if (callbacks->att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)callbacks->att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            ccc_val = mijia_get_ccc_value(conidx, att_idx);
            length = 2;
						// Send read response
						cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, length);
						cfm->handle = param->handle;
						cfm->status = status;
						cfm->length = length;
				 
						if (status == GAP_ERR_NO_ERROR)
						{
								memcpy(cfm->value, &ccc_val, length);
						}
				 
						ke_msg_send(cfm); 
        }
        else
        {
						arch_printf("read_val handle:%d\n",param->handle);
						
						mible_gatts_evt_t evt = MIBLE_GATTS_EVT_READ_PERMIT_REQ;
						mible_gatts_evt_param_t mi_param;
						mi_param.conn_handle = 0;
						mi_param.read.value_handle = param->handle - mijia_env->shdl;


						mible_gatts_event_callback(evt,&mi_param);
						
						//s_server_db
						//if(mi_param.read.permit == 0)
						//	status = PRF_APP_ERROR;
						//else
						//status = GAP_ERR_NO_ERROR;

						uint8_t* read_val = callbacks->att_db[att_idx].value;
            length = callbacks->att_db[att_idx].length;
						// Send read response
						cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, length);
						cfm->handle = param->handle;
						cfm->status = status;
						cfm->length = length;
				 
						if (status == GAP_ERR_NO_ERROR)
						{
								memcpy(cfm->value, read_val, length);
						}
				 
						ke_msg_send(cfm); 
						
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_REQ_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, const struct gattc_write_req_ind *param,
                                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    struct gattc_write_cfm * cfm;
    uint8_t att_idx = 0;
		arch_printf("gattc_write_req_ind_handler\n");
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = mijia_get_att_idx(param->handle, &att_idx);
    att_perm_type perm;

    ASSERT_ERR(param->offset == 0);
    // If the attribute has been found, status is ATT_ERR_NO_ERROR
    if (status == ATT_ERR_NO_ERROR)
    {
        const struct cust_prf_func_callbacks *callbacks = custs_get_func_callbacks(TASK_ID_MIJIA);

        if (callbacks->att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)callbacks->att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            struct attm_elmt elem = {0};

            // Find the handle of the Characteristic Value
            uint16_t value_hdl = get_value_handle(param->handle);
            ASSERT_ERR(value_hdl);
						arch_printf("param->handle:%d\n",param->handle);
            // Get permissions to identify if it is NTF or IND.
            attmdb_att_get_permission(value_hdl, &perm, 0, &elem);
            status = check_client_char_cfg(PERM_IS_SET(perm, NTF, ENABLE), param);
            //===
			      // Written value
            uint16_t ntf_cfg;
            uint16_t isEnable = true;
        
            // Extract value before check
            ntf_cfg = co_read16p(&param->value[0]);
            arch_printf("ntf_cfg:%d\n",ntf_cfg);
            // Only update configuration if value for stop or notification enable
            if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND) || (ntf_cfg == PRF_CLI_START_NTF))
            {
            	//Save value in DB
            	status = attmdb_att_set_value(param->handle, param->length, param->offset, (uint8_t *)&param->value[0]);
              arch_printf("status:%d\n",status);      
            	// Conserve information in environment
            	if (ntf_cfg == PRF_CLI_START_IND)
            	{
            		// Ntf cfg bit set to 1
            		mijia_env->feature |= PRF_CLI_START_IND;
            	}
							else if(ntf_cfg == PRF_CLI_START_NTF)
								mijia_env->feature |= PRF_CLI_START_NTF;	
            	else
            	{
            		isEnable = false;
            		// Ntf cfg bit set to 0
            		mijia_env->feature &= ~PRF_CLI_START_IND;
            	}                
							uint16_t val = isEnable;
							if(isEnable && (ntf_cfg == PRF_CLI_START_IND))
								val = 2;
            	mijia_enable_indication_notification(val,param->handle);
                    
            	status = ATT_ERR_NO_ERROR; 
                    
            }
//===	
            if (status == ATT_ERR_NO_ERROR)
            {
                mijia_set_ccc_value(conidx, att_idx, *(uint16_t *)param->value);
            }
        }
        else
        {
            if (callbacks != NULL && callbacks->value_wr_validation_func != NULL)
                status = callbacks->value_wr_validation_func(att_idx, param->offset, param->length, (uint8_t *)&param->value[0]);

            if (status == ATT_ERR_NO_ERROR)
            {
            		struct attm_elmt elem = {0};
            		// Find the handle of the Characteristic Value
		            uint16_t value_hdl = get_value_handle(param->handle);
		            //ASSERT_ERR(value_hdl);
		            attmdb_att_get_permission(value_hdl, &perm, 0, &elem);
								att_perm_type cmp_perm = PERM(WRITE_REQ, ENABLE);
								//
								mible_gatts_evt_t evt;
								if((perm & cmp_perm) == cmp_perm)
										evt = MIBLE_GATTS_EVT_WRITE;
								else
										evt = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;

								bool has_write = false;
								if((evt == MIBLE_GATTS_EVT_WRITE) || (!get_wr_author(param->handle-mijia_env->shdl)))
								{
										has_write = true;
                		// Set value in the database
                		status = attmdb_att_set_value(param->handle, param->length, param->offset, (uint8_t *)&param->value[0]);
								}
										
								mible_gatts_evt_param_t mi_param;
								memset(&mi_param,0,sizeof(mi_param));
								mi_param.conn_handle = 0;
								mi_param.write.len = param->length;
								mi_param.write.data = (uint8_t*)param->value;
								mi_param.write.value_handle = param->handle - mijia_env->shdl;

								mible_gatts_event_callback(evt,&mi_param);
								
								//if((mi_param.write.permit != 0) && ((!has_write)))
								if(!has_write)
								{
										has_write = true;
										status = attmdb_att_set_value(param->handle, param->length, param->offset, (uint8_t *)&param->value[0]);
								}
								else
								{
										if(!has_write)
											status = PRF_APP_ERROR;
								}
						}
        }

    }

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


//直接设置值
mible_status_t mijia_gatts_value_set_direct(uint16_t srv_handle,
uint16_t value_handle, uint8_t offset,uint8_t* p_value, uint8_t len)
{
		if(p_value == NULL)
				return MI_ERR_INVALID_PARAM;
		
		struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
		uint16_t length;
    uint8_t *value;
		//读取之前的数据
		uint8_t state = attmdb_get_value(mijia_env->shdl+value_handle,&length,&value);
		if(ATT_ERR_NO_ERROR == state){
				uint8_t *pbuf = ke_malloc(length*sizeof(uint8_t), KE_MEM_NON_RETENTION); 
				if(pbuf != NULL){
						memcpy(pbuf,value,length);
						//拼接数据
						if(offset + len > length)
								return MI_ERR_INVALID_PARAM;
						else
								memcpy(pbuf+offset,p_value,len);
						
						//将数据写回去
						state = attmdb_att_set_value(value_handle+mijia_env->shdl,length,0,pbuf);
						if(state != ATT_ERR_NO_ERROR)
								return MIBLE_ERR_UNKNOWN;

						return MI_SUCCESS;
				}
				else
						return MI_ERR_NO_MEM;
		}
		else
				return MI_ERR_INVALID_PARAM;
		
}




/**
 ****************************************************************************************
 * @brief Send notifcation to peer device
 * @param[in] msgid 		Id of the message received.
 * @param[in] param 		Pointer to the parameters of the message.
 * @param[in] dest_id 	ID of the receiving task instance
 * @param[in] src_id		ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
mible_status_t mijia_send_notifcation_req_handler_direct(ke_msg_id_t const msgid,
																		struct mijia_notifcation_req const *param,
																		ke_task_id_t const dest_id,
																		ke_task_id_t const src_id)
{
		mible_status_t status = MI_SUCCESS;
		struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
		// Check provided values
		if(param->conhdl == KE_IDX_GET(src_id))
		{
				//if((mijia_env->feature & PRF_CLI_START_IND))
				{
						attmdb_att_set_value(param->value_handle+mijia_env->shdl, 
																				param->length,0,
																				(uint8_t *)(param->value));
						uint16_t cfg_hdl;
						// Find the handle of the Characteristic Client Configuration
						cfg_hdl = get_cfg_handle(param->value_handle+mijia_env->shdl);

						// Send indication through GATT
						uint16_t ccc_val = 0;
						ccc_val = mijia_get_ccc_value(0, cfg_hdl-mijia_env->shdl);
						if(ccc_val == PRF_CLI_START_NTF || ccc_val == PRF_CLI_START_IND)
						{
							 // Allocate the GATT notification message
								struct gattc_send_evt_cmd *req = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
										KE_BUILD_ID(TASK_GATTC,mijia_env->cursor), dest_id,
										gattc_send_evt_cmd,param->length);

								// Fill in the parameter structure
								if(ccc_val == PRF_CLI_START_NTF)
										req->operation = GATTC_NOTIFY;
								else
										req->operation = PRF_CLI_START_IND;
								req->handle = param->value_handle+mijia_env->shdl;
								req->length = param->length;
								memcpy(req->value,param->value,param->length);
								// Send the event
								ke_msg_send(req);
						}
				} 			 
		}
		else
		{
				COMPrintf("MI_ERR_INVALID_PARAM\n");
				status = MI_ERR_INVALID_PARAM;
		}


		return status;
}

/**
 ****************************************************************************************
 * @brief Send notifcation to peer device
 * @param[in] msgid 		Id of the message received.
 * @param[in] param 		Pointer to the parameters of the message.
 * @param[in] dest_id 	ID of the receiving task instance
 * @param[in] src_id		ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int mijia_send_notifcation_req_handler(ke_msg_id_t const msgid,
																		struct mijia_notifcation_req const *param,
																		ke_task_id_t const dest_id,
																		ke_task_id_t const src_id)
{
		uint8_t status = ATT_ERR_NO_ERROR;
	  struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
		// Check provided values
		if(param->conhdl == KE_IDX_GET(src_id))
		{
				//if((mijia_env->feature & PRF_CLI_START_IND))
				{
						attmdb_att_set_value(param->value_handle+mijia_env->shdl, 
																				param->length,0,
																				(uint8_t *)(param->value));
						uint16_t cfg_hdl;
						// Find the handle of the Characteristic Client Configuration
		    		cfg_hdl = get_cfg_handle(param->value_handle+mijia_env->shdl);

						// Send indication through GATT
						uint16_t ccc_val = 0;
						ccc_val = mijia_get_ccc_value(0, cfg_hdl-mijia_env->shdl);
						if(ccc_val == PRF_CLI_START_NTF || ccc_val == PRF_CLI_START_IND)
						{
							 // Allocate the GATT notification message
								struct gattc_send_evt_cmd *req = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
										KE_BUILD_ID(TASK_GATTC,mijia_env->cursor), dest_id,
										gattc_send_evt_cmd,param->length);

								// Fill in the parameter structure
								if(ccc_val == PRF_CLI_START_NTF)
										req->operation = GATTC_NOTIFY;
								else
										req->operation = PRF_CLI_START_IND;
								req->handle = param->value_handle+mijia_env->shdl;
								req->length = param->length;
								memcpy(req->value,param->value,param->length);
								// Send the event
								ke_msg_send(req);
						}
				} 			 
		}
		else
		{
				status = PRF_ERR_INVALID_PARAM;
		}

		if (status != ATT_ERR_NO_ERROR)
		{
				mijia_ind_ntf_cfm_send(status);
		}

		return (KE_MSG_CONSUMED);
}
	

/**
 ****************************************************************************************
 * @brief Send indication to peer device
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int mijia_send_indication_req_handler(ke_msg_id_t const msgid,
                                    struct mijia_indication_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t status = ATT_ERR_NO_ERROR;
	struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    // Check provided values
    if(param->conhdl == KE_IDX_GET(src_id))
    {
        if((mijia_env->feature & PRF_CLI_START_IND))
        {
            attmdb_att_set_value(mijia_env->shdl + param->value_handle, 
                                        param->length,0,
                                        (uint8_t *)(param->value));
                    
			 // Allocate the GATT notification message
    		struct gattc_send_evt_cmd *req = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC,mijia_env->cursor), dest_id,
            gattc_send_evt_cmd,param->length);

    		// Fill in the parameter structure
    		req->operation = GATTC_INDICATE;
    		req->handle = mijia_env->shdl + param->value_handle;
			  req->length = param->length;
			  memcpy(req->value,param->value,param->length);
    		// Send the event
    		ke_msg_send(req);
        }        
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != ATT_ERR_NO_ERROR)
    {
        mijia_ind_ntf_cfm_send(status);
    }

    return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler mijia_default_state[] =
{
    {GATTC_READ_REQ_IND,            (ke_msg_func_t)gattc_read_req_ind_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t)gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
	  {MIJIA_SEND_INDICATION_REQ,     (ke_msg_func_t)mijia_send_indication_req_handler},
    {MIJIA_SEND_NOTIFCATION_REQ,		(ke_msg_func_t)mijia_send_notifcation_req_handler},
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler mijia_default_handler = KE_STATE_HANDLER(mijia_default_state);

#endif // BLE_MIJIA_SERVER
