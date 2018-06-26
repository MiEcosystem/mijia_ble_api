/**
 ****************************************************************************************
 *
 * @file sps_task.c
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

#if (BLE_SPS_SERVER)
#include "sps_task.h"
#include "sps.h"
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

#if (BLE_CUSTOM_SERVER)
#include "user_custs_config.h"
#include "mi_app_customs.h"
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
static int sps_att_set_value(uint8_t att_idx, uint16_t length, const uint8_t *data)
{
    struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
    // Check value in already present in service
    struct sps_val_elmt *val = (struct sps_val_elmt *) co_list_pick(&(sps_env->values));
    // loop until value found
    while (val != NULL)
    {
        // value is present in service
        if (val->att_idx == att_idx)
        {
            // Value present but size changed, free old value
            if (length != val->length)
            {
                co_list_extract(&sps_env->values, &val->hdr, 0);
                ke_free(val);
                val = NULL;
            }
            break;
        }

        val = (struct sps_val_elmt *)val->hdr.next;
    }

    if (val == NULL)
    {
        // allocate value data
        val = (struct sps_val_elmt *) ke_malloc(sizeof(struct sps_val_elmt) + length, KE_MEM_ATT_DB);
        // insert value into the list
        co_list_push_back(&sps_env->values, &val->hdr);
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
static int sps_att_get_value(uint8_t att_idx, uint16_t *length, const uint8_t **data)
{
    struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
    // Check value in already present in service
    struct sps_val_elmt *val = (struct sps_val_elmt *) co_list_pick(&sps_env->values);
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

        val = (struct sps_val_elmt *)val->hdr.next;
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
void sps_init_ccc_values(const struct attm_desc_128 *att_db, int max_nb_att)
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
            sps_att_set_value(i, sizeof(ccc_values), ccc_values);
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
void sps_set_ccc_value(uint8_t conidx, uint8_t att_idx, uint16_t ccc)
{
    uint16_t length;
    const uint8_t *value;
    uint8_t new_value[BLE_CONNECTION_MAX];
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    sps_att_get_value(att_idx, &length, &value);
    ASSERT_ERR(length);
    ASSERT_ERR(value);
    memcpy(new_value, value, length);
    // For now there are only two valid values for ccc, store just one byte other is 0 anyway
    new_value[conidx] = (uint8_t)ccc;
    sps_att_set_value(att_idx, length, new_value);
}

/**
 ****************************************************************************************
 * @brief Read value of CCC for given attribute and connection index.
 * @param[in]  conidx   Connection index.
 * @param[in]  att_idx  Custom attribute index.
 * @return Value of CCC.
 ****************************************************************************************
 */
static uint16_t sps_get_ccc_value(uint8_t conidx, uint8_t att_idx)
{
    uint16_t length;
    const uint8_t *value;
    uint16_t ccc_value;

    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    sps_att_get_value(att_idx, &length, &value);
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
    struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);

    if (param->operation == GATTC_INDICATE || param->operation == GATTC_NOTIFY)
    {
       if (FirstNotificationBit == 0)
            SecondNotificationBit = 0;
            FirstNotificationBit = 0;
            sps_indication_cfm_send(param->status);
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
    struct gattc_read_cfm * cfm;
    uint8_t att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = sps_get_att_idx(param->handle, &att_idx);
    uint16_t length = 0;
    uint16_t ccc_val = 0;
    struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);

    // If the attribute has been found, status is GAP_ERR_NO_ERROR
    if (status == GAP_ERR_NO_ERROR)
    {
        const struct cust_prf_func_callbacks *callbacks = custs_get_func_callbacks(TASK_ID_SPS);

        if (callbacks->att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)callbacks->att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            ccc_val = sps_get_ccc_value(conidx, att_idx);
            length = 2;
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }

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
    struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
    struct gattc_write_cfm * cfm;
    uint8_t att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = sps_get_att_idx(param->handle, &att_idx);
    att_perm_type perm;
		

    ASSERT_ERR(param->offset == 0);
    // If the attribute has been found, status is ATT_ERR_NO_ERROR
    if (status == ATT_ERR_NO_ERROR)
    {
        const struct cust_prf_func_callbacks *callbacks = custs_get_func_callbacks(TASK_ID_SPS);

        if (callbacks->att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)callbacks->att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            struct attm_elmt elem = {0};

            // Find the handle of the Characteristic Value
            uint16_t value_hdl = get_value_handle(param->handle);
            ASSERT_ERR(value_hdl);

            // Get permissions to identify if it is NTF or IND.
            attmdb_att_get_permission(value_hdl, &perm, 0, &elem);
            status = check_client_char_cfg(PERM_IS_SET(perm, NTF, ENABLE), param);
//===
			// Written value
            uint16_t ntf_cfg;
            uint16_t isEnable = true;
        
            // Extract value before check
            ntf_cfg = co_read16p(&param->value[0]);
                
            // Only update configuration if value for stop or notification enable
            if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND) || (ntf_cfg == PRF_CLI_START_NTF))
            {
            	//Save value in DB
            	status = attmdb_att_set_value(param->handle, param->length, param->offset, (uint8_t *)&param->value[0]);
                    
            	// Conserve information in environment
            	if (ntf_cfg == PRF_CLI_START_IND)
            	{
            		// Ntf cfg bit set to 1
            		sps_env->feature |= PRF_CLI_START_IND;
            	}
							else if(ntf_cfg == PRF_CLI_START_NTF)
							{
								// Ntf cfg bit set to 1
            		sps_env->feature |= PRF_CLI_START_NTF;
							}
            	else
            	{
            		isEnable = false;
            		// Ntf cfg bit set to 0
            		sps_env->feature = PRF_CLI_STOP_NTFIND;
            	}                
                    
            	sps_enable_indication(isEnable);
                    
            	status = ATT_ERR_NO_ERROR; 
                    
            }
//===	
            if (status == ATT_ERR_NO_ERROR)
            {
                sps_set_ccc_value(conidx, att_idx, *(uint16_t *)param->value);
            }
        }
        else
        {
            if (callbacks != NULL && callbacks->value_wr_validation_func != NULL)
                status = callbacks->value_wr_validation_func(att_idx, param->offset, param->length, (uint8_t *)&param->value[0]);

            if (status == ATT_ERR_NO_ERROR)
            {
                // Set value in the database
                status = attmdb_att_set_value(param->handle, param->length, param->offset, (uint8_t *)&param->value[0]);
								sps_send_write_val((uint8_t *)(param->value), param->length);
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
static int sps_send_indication_req_handler(ke_msg_id_t const msgid,
                                    struct sps_indication_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint8_t status = ATT_ERR_NO_ERROR;
	struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
    // Check provided values
    if(param->conhdl == KE_IDX_GET(src_id))
    {

        if((sps_env->feature & PRF_CLI_START_IND) || (sps_env->feature & PRF_CLI_START_NTF))
        {
            attmdb_att_set_value(sps_env->shdl + SPS_IDX_IND_VAL, 
                                        param->length,0,
                                        (uint8_t *)(param->value));

					 // Allocate the GATT notification message
		    		struct gattc_send_evt_cmd *req = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
		            KE_BUILD_ID(TASK_GATTC,sps_env->cursor), dest_id,
		            gattc_send_evt_cmd,param->length);

		    		// Fill in the parameter structure
		    		if(sps_env->feature & PRF_CLI_START_IND)
		    			req->operation = GATTC_INDICATE;
						else
							req->operation = GATTC_NOTIFY;
		    		req->handle = sps_env->shdl + SPS_IDX_IND_VAL;
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
        sps_indication_cfm_send(status);
    }

    return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/**** NOTIFICATION BITS ***********/
uint8_t FirstNotificationBit __attribute__((section("retention_mem_area0"), zero_init));
uint8_t SecondNotificationBit __attribute__((section("retention_mem_area0"), zero_init));


/// Default State handlers definition
const struct ke_msg_handler sps_default_state[] =
{
    {GATTC_READ_REQ_IND,            (ke_msg_func_t)gattc_read_req_ind_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t)gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
	  {SPS_SEND_REQ,       						(ke_msg_func_t)sps_send_indication_req_handler},
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler sps_default_handler = KE_STATE_HANDLER(sps_default_state);

#endif // BLE_SPS_SERVER
