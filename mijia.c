/**
 ****************************************************************************************
 *
 * @file mijia.c
 *
 * @brief Custom Service profile source file.
 *
 * Copyright (C) 2017 Dialog Semiconductor.
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

#include "rwip_config.h"              // SW configuration

#if (BLE_MIJIA_SERVER)
#include "mijia.h"
#include "mijia_task.h"
#include "attm_db.h"
#include "gapc.h"
#include "prf.h"
#include "prf_utils.h"
#include "arch_console.h"
#include "ke_mem.h"
#include "mi_attm_db_128.h"

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static uint16_t att_decl_svc       			= ATT_DECL_PRIMARY_SERVICE;
static uint16_t att_decl_secondry_svc   = ATT_DECL_SECONDARY_SERVICE;

static uint16_t att_decl_char      			= ATT_DECL_CHARACTERISTIC;
static uint16_t att_decl_cfg       			= ATT_DESC_CLIENT_CHAR_CFG;

//米家服务 特征值数量
static uint16_t s_max_mijia_idx_nb = 0;


//按照给应用层的句柄对应
static bool s_wr_author[MIJIA_IDX_ATT_DB_MAX] = {0};

/**
 ****************************************************************************************
 * @brief Initialization of the MIJIA module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 *
 * @param[out]    env        Collector or Service allocated environment data.
 * @param[in|out] start_hdl  Service start handle (0 - dynamically allocated), only applies for services.
 * @param[in]     app_task   Application task number.
 * @param[in]     sec_lvl    Security level (AUTH, EKS and MI field of @see enum attm_value_perm_mask)
 * @param[in]     param      Configuration parameters of profile collector or service (32 bits aligned)
 *
 * @return status code to know if profile initialization succeed or not.
 ****************************************************************************************
 */
static uint8_t mijia_init(struct prf_task_env *env, uint16_t *start_hdl, uint16_t app_task, uint8_t sec_lvl, struct mijia_db_cfg *params)
{
    //------------------ create the attribute database for the profile -------------------

    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    status = attm_svc_create_db_mijia(start_hdl, NULL,
            s_max_mijia_idx_nb, NULL, env->task, mijia_att_db,
            (sec_lvl & PERM_MASK_SVC_AUTH) | (sec_lvl & PERM_MASK_SVC_EKS) | PERM(SVC_PRIMARY, ENABLE));


    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        struct mijia_env_tag *mijia_env =
                (struct mijia_env_tag *) ke_malloc(sizeof(struct mijia_env_tag), KE_MEM_ATT_DB);

        // allocate MIJIA required environment variable
        env->env = (prf_env_t *)mijia_env;
        mijia_env->shdl = *start_hdl;
        mijia_env->max_nb_att = s_max_mijia_idx_nb;
        mijia_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        mijia_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_MIJIA;
        env->desc.idx_max           = MIJIA_IDX_MAX;
        env->desc.state             = mijia_env->state;
        env->desc.default_handler   = &mijia_default_handler;
        co_list_init(&(mijia_env->values));
        mijia_init_ccc_values(mijia_att_db, s_max_mijia_idx_nb);

        // service is ready, go into an Idle state
        ke_state_set(env->task, MIJIA_IDLE);
    }

		
    return status;
}
/**
 ****************************************************************************************
 * @brief Destruction of the MIJIA module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void mijia_destroy(struct prf_task_env *env)
{
    struct mijia_env_tag *mijia_env = (struct mijia_env_tag *)env->env;

    // remove all values present in list
    while (!co_list_is_empty(&(mijia_env->values)))
    {
        struct co_list_hdr *hdr = co_list_pop_front(&(mijia_env->values));
        ke_free(hdr);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(mijia_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void mijia_create(struct prf_task_env *env, uint8_t conidx)
{
    int att_idx;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // Find all ccc fields and clean them
    for (att_idx = 1; att_idx < s_max_mijia_idx_nb; att_idx++)
    {
        // Find only CCC characteristics
        if (mijia_att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)mijia_att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            // Clear CCC value
            mijia_set_ccc_value(conidx, att_idx, 0);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Handles Disconnection
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void mijia_cleanup(struct prf_task_env *env, uint8_t conidx, uint8_t reason)
{
    int att_idx;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // Find all ccc fields and clean them
    for (att_idx = 1; att_idx < s_max_mijia_idx_nb; att_idx++)
    {
        // Find only CCC characteristics
        if (mijia_att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)mijia_att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            // Clear CCC value
            mijia_set_ccc_value(conidx, att_idx, 0);
        }
    }
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// MIJIA Task interface required by profile manager
const struct prf_task_cbs mijia_itf =
{
        (prf_init_fnct) mijia_init,
        mijia_destroy,
        mijia_create,
        mijia_cleanup,
};


struct attm_desc_128 mijia_att_db[MIJIA_IDX_ATT_DB_MAX];

static mible_status_t add_char_declaration(mible_gatts_char_db_t *inchar,struct attm_desc_128 *outatt)
{
		if(inchar==NULL || outatt == NULL)
			return MI_ERR_INTERNAL;

		outatt->uuid = (uint8_t*)&att_decl_char;
		outatt->uuid_size = ATT_UUID_16_LEN;

		outatt->perm = PERM(RD, ENABLE);
		
		outatt->max_length = sizeof(struct att_char_desc);
		outatt->length = sizeof(struct att_char_desc);

		//16bit  uuid
		if(inchar->char_uuid.type == 0)
		{
				struct att_char_desc *pchar_desc = (struct att_char_desc*)ke_malloc(sizeof(struct att_char_desc), KE_MEM_NON_RETENTION);
				ASSERT_ERROR(pchar_desc != NULL);
				memset(pchar_desc->attr_hdl,0,sizeof(pchar_desc->attr_hdl));
				memcpy(pchar_desc->attr_type,&(inchar->char_uuid.uuid16),sizeof(pchar_desc->attr_type));
				pchar_desc->prop = inchar->char_property;
				outatt->value = (uint8_t*)(pchar_desc);
		}
		else 
		{
				struct att_char128_desc *pchar_desc = (struct att_char128_desc*)ke_malloc(sizeof(struct att_char128_desc), KE_MEM_NON_RETENTION);
				memset(pchar_desc->attr_hdl,0,sizeof(pchar_desc->attr_hdl));
				memcpy(pchar_desc->attr_type,&(inchar->char_uuid.uuid128),sizeof(pchar_desc->attr_type));
				pchar_desc->prop = inchar->char_property;
				outatt->value = (uint8_t*)(pchar_desc);
		}

		
		
		return MI_SUCCESS;
		
}

static mible_status_t add_char_cfg(mible_gatts_char_db_t *inchar,struct attm_desc_128 *outatt)
{
		if(inchar==NULL || outatt == NULL)
			return MI_ERR_INTERNAL;
		
		outatt->uuid = (uint8_t*)&att_decl_cfg;
		outatt->uuid_size = ATT_UUID_16_LEN;

		outatt->perm = (PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE));
		
		outatt->max_length = sizeof(uint16_t);
		outatt->length = 0;
		outatt->value = NULL;

		return MI_SUCCESS;
}


//truct anpc_env_tag* anpc_env = ke_malloc(sizeof(struct anpc_env_tag), KE_MEM_ATT_DB);
static mible_status_t add_char_value(mible_gatts_char_db_t *inchar,struct attm_desc_128 *outatt)
{
		if(inchar==NULL || outatt == NULL)
			return MI_ERR_INTERNAL;

		uint8_t* puuid = (uint8_t*)ke_malloc(ATT_UUID_128_LEN, KE_MEM_ATT_DB);
		if(inchar->char_uuid.type == 0)
		{
				outatt->uuid_size = ATT_UUID_16_LEN;
				memcpy(puuid,&(inchar->char_uuid.uuid16),ATT_UUID_16_LEN);
				outatt->uuid = puuid;
		}
		else
		{
				outatt->uuid_size = ATT_UUID_128_LEN;
				memcpy(puuid,&(inchar->char_uuid.uuid128[0]),ATT_UUID_128_LEN);
				outatt->uuid = puuid;
		}
		
		if((inchar->char_property & MIBLE_READ)  > 0)
			outatt->perm |= PERM(RD, ENABLE);

		if((inchar->char_property & MIBLE_WRITE) > 0)
			outatt->perm |= (PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE));	


		if((inchar->char_property & MIBLE_WRITE_WITHOUT_RESP) > 0)
			outatt->perm |= (PERM(WR, ENABLE) | PERM(WRITE_COMMAND, ENABLE));

		if((inchar->char_property & MIBLE_NOTIFY)  > 0)
			outatt->perm |= PERM(NTF, ENABLE);

		if((inchar->char_property & MIBLE_INDICATE)  > 0)
			outatt->perm |= PERM(IND, ENABLE);

		outatt->max_length = inchar->char_value_len;
		if(inchar->rd_author)
			outatt->max_length |= PERM(RI, ENABLE);
		
		outatt->length	= inchar->char_value_len;//
		uint8_t* pval = (uint8_t*)ke_malloc(outatt->length, KE_MEM_ATT_DB);
		memcpy(pval,inchar->p_value,inchar->char_value_len);
		outatt->value = pval;

		
		
		return MI_SUCCESS;
}



//
static mible_status_t  translate_char_db(mible_gatts_char_db_t *inchar,struct attm_desc_128 *outatt,uint16_t *out_att_pos)
{

		//增加特征值声明
		mible_status_t sta = add_char_declaration(inchar,&outatt[(*out_att_pos)]);
		(*out_att_pos)++;
		if(sta != MI_SUCCESS)
				return sta;
		

		//增加特征值的值
		sta = add_char_value(inchar,&outatt[(*out_att_pos)]);
		s_wr_author[(*out_att_pos)] = inchar->wr_author;
		inchar->char_value_handle = *out_att_pos;
		(*out_att_pos)++;
		if(sta != MI_SUCCESS)
				return sta;
		

		//增加ccc
		if( ((inchar->char_property & MIBLE_NOTIFY) > 0) ||\
			  ((inchar->char_property & MIBLE_INDICATE) > 0))
		{
				sta = add_char_cfg(inchar,&outatt[(*out_att_pos)]);
				(*out_att_pos)++;
				if(sta != MI_SUCCESS)
					return sta;
		}
		
		return MI_SUCCESS;
}

static mible_status_t  translate_service_db(mible_gatts_srv_db_t *inserver,struct attm_desc_128 *outatt)
{
		mible_status_t sta = MI_SUCCESS;
		//inserver->srv_type
		if(inserver != NULL)
		{
				uint8_t* puuid = (uint8_t*)ke_malloc(ATT_UUID_16_LEN, KE_MEM_NON_RETENTION);
				outatt->uuid_size = ATT_UUID_16_LEN;
				if(inserver->srv_type == MIBLE_PRIMARY_SERVICE)
						memcpy(puuid,&(att_decl_svc),ATT_UUID_16_LEN);
				else
						memcpy(puuid,&(att_decl_secondry_svc),ATT_UUID_16_LEN);
				
				outatt->uuid = puuid;

				outatt->perm = PERM(RD, ENABLE);
				outatt->max_length = outatt->uuid_size;
				outatt->length = outatt->uuid_size;
				

				if(inserver->srv_uuid.type == 0)
				{
						puuid = (uint8_t*)ke_malloc(ATT_UUID_16_LEN, KE_MEM_NON_RETENTION);
						memcpy(puuid,&(inserver->srv_uuid.uuid16),ATT_UUID_16_LEN);
						outatt->value = puuid;
				}
				else
				{
						puuid = (uint8_t*)ke_malloc(ATT_UUID_128_LEN, KE_MEM_NON_RETENTION);
						memcpy(puuid,&(inserver->srv_uuid.uuid128),ATT_UUID_128_LEN);
						outatt->value = puuid;
				}
		}
		else
				sta = MI_ERR_INVALID_PARAM;
		
		return sta;

}


static mible_gatts_db_t * s_pserver_db __attribute__((section("retention_mem_area0"),zero_init));
mible_gatts_db_t *get_server_db(void)
{
		return s_pserver_db;
}


bool get_wr_author(uint16_t value_handle)
{
		if(value_handle < MIJIA_IDX_ATT_DB_MAX)
				return s_wr_author[value_handle];
		else
				return false;
}

//将米家平台配置的GATT属性表转换 dialog 平台的
mible_status_t translate_miarch_attdb(mible_gatts_db_t * p_server_db)
{
			s_pserver_db = p_server_db;
			
			mible_status_t ret = MI_SUCCESS;
		
			if(p_server_db != NULL)
			{
					uint8_t srv_num = p_server_db->srv_num;
					uint16_t pos = 0;
					for(int i=0;(i<srv_num) &&(ret == MI_SUCCESS);i++)
					{
							uint8_t char_num = p_server_db->p_srv_db[i].char_num;
							translate_service_db(p_server_db->p_srv_db,&mijia_att_db[pos]);
							p_server_db->p_srv_db->srv_handle = pos;
							pos++;
							for(int j=0;j<char_num;j++)
							{
									mible_gatts_char_db_t *pchar = &p_server_db->p_srv_db[i].p_char_db[j];
									
									ret = translate_char_db(pchar,&mijia_att_db[0],&pos);
									if(ret != MI_SUCCESS)
											break;
							}
					}

					s_max_mijia_idx_nb =  pos;
			}
			else
					ret = MI_ERR_INVALID_PARAM;

		
			return ret;
}



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* mijia_prf_itf_get(void)
{
    return &mijia_itf;
}



void mijia_send_write_val(mible_gatts_evt_t evt,uint16_t value_handle,uint8_t* val, uint32_t length)
{

		
	struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    // Allocate the write value indication message
   struct mijia_val_write_ind *ind = KE_MSG_ALLOC(MIJIA_VAL_WRITE_IND,
                                              prf_dst_task_get(&(mijia_env->prf_env), mijia_env->cursor), 
                                              prf_src_task_get(&(mijia_env->prf_env), mijia_env->cursor),
                                              mijia_val_write_ind);
   // Fill in the parameter structure
   ind->conhdl = gapc_get_conhdl(mijia_env->cursor);
   memcpy(ind->value, val, length);
	 ind->evt = evt;
   ind->length = length;
   ind->value_handle = value_handle;
   // Send the message
   ke_msg_send(ind);
}



void mijia_enable_indication_notification(uint16_t isEnable,uint16_t handle)
{
		
		struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    // Allocate the enable indication request message
   struct mijia_enable_indication_req *ind = KE_MSG_ALLOC(MIJIA_ENABLE_IND_NTF_REQ,
                                              prf_dst_task_get(&(mijia_env->prf_env), mijia_env->cursor), 
                                              prf_src_task_get(&(mijia_env->prf_env), mijia_env->cursor),
                                              mijia_enable_indication_req);
   // Fill in the parameter structure
   ind->conhdl = gapc_get_conhdl(mijia_env->cursor);
   ind->isEnable = isEnable;
   ind->value_handle = handle-mijia_env->shdl;
   // Send the message
   ke_msg_send(ind);
}

void mijia_ind_ntf_cfm_send(uint8_t status)
{
		struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
	// allocate indication confirmation message to tell application layer the host has received the indication sent from device
    struct mijia_indication_cfm *cfm = KE_MSG_ALLOC(MIJIA_SEND_INDICATION_CFM,
                                                 prf_dst_task_get(&(mijia_env->prf_env), mijia_env->cursor), 
																								 prf_src_task_get(&(mijia_env->prf_env), mijia_env->cursor),
                                                 mijia_indication_cfm);
    cfm->status = status;
    // Send the message
    ke_msg_send(cfm);
}

#endif // (BLE_MIJIA_SERVER)
