/**
 ****************************************************************************************
 *
 * @file sps.c
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

#if (BLE_SPS_SERVER)
#include "sps.h"
#include "sps_task.h"
#include "attm_db.h"
#include "gapc.h"
#include "prf.h"
#include "prf_utils.h"
#include "mi_attm_db_128.h"


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#if (BLE_SPS_SERVER)
#define ATT_CHAR(prop, handle, type) {(prop),                                           \
                                     {(uint8_t)((handle)), (uint8_t)((handle) >> 8)},  \
                                     {(uint8_t)((type)), (uint8_t)((type) >> 8)}       };
#endif //BLE_SPS_SERVER
																		 
static att_svc_desc128_t sps_svc 	= BLE_UUID_SPS_SERVICE;
static uint16_t att_decl_svc       	= ATT_DECL_PRIMARY_SERVICE;
static uint16_t att_decl_char      	= ATT_DECL_CHARACTERISTIC;
static uint16_t att_decl_cfg       	= ATT_DESC_CLIENT_CHAR_CFG;
static uint16_t sps_uuid_write     = BLE_UUID_SPS_WRITE_CHARACTERISTICS;
static uint16_t sps_uuid_read      = BLE_UUID_SPS_READ_CHARACTERISTICS;
static uint16_t sps_uuid_ind      	= BLE_UUID_SPS_NOTIFY_CHARACTERISTICS;


/**
 ****************************************************************************************
 * @brief Initialization of the SPS module.
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
static uint8_t sps_init(struct prf_task_env *env, uint16_t *start_hdl, uint16_t app_task, uint8_t sec_lvl, struct sps_db_cfg *params)
{
    //------------------ create the attribute database for the profile -------------------

    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    status = attm_svc_create_db_sps(start_hdl, NULL,
            SPS_IDX_NB, NULL, env->task, sps_att_db,
            (sec_lvl & PERM_MASK_SVC_AUTH) | (sec_lvl & PERM_MASK_SVC_EKS) | PERM(SVC_PRIMARY, ENABLE));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        struct sps_env_tag *sps_env =
                (struct sps_env_tag *) ke_malloc(sizeof(struct sps_env_tag), KE_MEM_ATT_DB);

        // allocate SPS required environment variable
        env->env = (prf_env_t *)sps_env;
        sps_env->shdl = *start_hdl;
        sps_env->max_nb_att = SPS_IDX_NB;
        sps_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        sps_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_SPS;
        env->desc.idx_max           = SPS_IDX_MAX;
        env->desc.state             = sps_env->state;
        env->desc.default_handler   = &sps_default_handler;
				sps_env->feature = 0;
        co_list_init(&(sps_env->values));
				
        sps_init_ccc_values(sps_att_db, SPS_IDX_NB);

        // service is ready, go into an Idle state
        ke_state_set(env->task, SPS_IDLE);
    }

    return status;
}
/**
 ****************************************************************************************
 * @brief Destruction of the SPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void sps_destroy(struct prf_task_env *env)
{
    struct sps_env_tag *sps_env = (struct sps_env_tag *)env->env;

    // remove all values present in list
    while (!co_list_is_empty(&(sps_env->values)))
    {
        struct co_list_hdr *hdr = co_list_pop_front(&(sps_env->values));
        ke_free(hdr);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(sps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void sps_create(struct prf_task_env *env, uint8_t conidx)
{
    int att_idx;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // Find all ccc fields and clean them
    for (att_idx = 1; att_idx < SPS_IDX_NB; att_idx++)
    {
        // Find only CCC characteristics
        if (sps_att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)sps_att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            // Clear CCC value
            sps_set_ccc_value(conidx, att_idx, 0);
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
static void sps_cleanup(struct prf_task_env *env, uint8_t conidx, uint8_t reason)
{
    int att_idx;
    ASSERT_ERR(conidx < BLE_CONNECTION_MAX);

    // Find all ccc fields and clean them
    for (att_idx = 1; att_idx < SPS_IDX_NB; att_idx++)
    {
        // Find only CCC characteristics
        if (sps_att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)sps_att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            // Clear CCC value
            sps_set_ccc_value(conidx, att_idx, 0);
        }
    }

}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// SPS Task interface required by profile manager
const struct prf_task_cbs sps_itf =
{
        (prf_init_fnct) sps_init,
        sps_destroy,
        sps_create,
        sps_cleanup,
};

/// Full CUSTOM1 Database Description - Used to add attributes into the database
struct attm_desc_128 sps_att_db[SPS_IDX_NB] =
{
     // Sps Service Declaration
    [SPS_IDX_SVC]                  =   {(uint8_t*)&att_decl_svc,ATT_UUID_16_LEN, PERM(RD, ENABLE), sizeof(sps_svc),
                                        sizeof(sps_svc), (uint8_t*)&sps_svc},

   // Sps Write Characteristic Declaration
    [SPS_IDX_WRITE_CHAR]        =   {(uint8_t*)&att_decl_char, ATT_UUID_16_LEN,PERM(RD, ENABLE), sizeof(sps_write_char),
                                         sizeof(sps_write_char), (uint8_t *)&sps_write_char},
    // Sps Write Characteristic Value
    [SPS_IDX_WRITE_VAL]         =   {(uint8_t*)&sps_uuid_write, ATT_UUID_16_LEN,PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE), BLE_SPS_MAX_DATA_LEN,
                                         0, NULL},
  
    // Sps Read Characteristic Declaration
    [SPS_IDX_READ_CHAR]        =   {(uint8_t*)&att_decl_char, ATT_UUID_16_LEN,PERM(RD, ENABLE), sizeof(sps_read_char),
                                         sizeof(sps_read_char), (uint8_t *)&sps_read_char},
    // Sps Read Characteristic Value
    [SPS_IDX_READ_VAL]         =   {(uint8_t*)&sps_uuid_read, ATT_UUID_16_LEN,PERM(RD, ENABLE), BLE_SPS_MAX_DATA_LEN,
                                         0, NULL},
    
    // Sps IND Characteristic Declaration
    [SPS_IDX_IND_CHAR]        =   {(uint8_t*)&att_decl_char, ATT_UUID_16_LEN,PERM(RD, ENABLE), sizeof(sps_ntf_char),
                                        sizeof(sps_ntf_char), (uint8_t *)&sps_ntf_char},
    // Sps IND Characteristic Value
    [SPS_IDX_IND_VAL]         =   {(uint8_t*)&sps_uuid_ind, ATT_UUID_16_LEN, PERM(NTF, ENABLE), BLE_SPS_MAX_DATA_LEN,
                                        0, NULL},
    // Sps IND Characteristic - Client Characteristic Configuration Descriptor
    [SPS_IDX_IND_CFG]     =   {(uint8_t*)&att_decl_cfg, ATT_UUID_16_LEN,PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE), sizeof(uint16_t),
                                        0, NULL},
		
};




/// Sps write characteristic
const struct att_char_desc sps_write_char = ATT_CHAR(ATT_CHAR_PROP_WR,
                                                            0,
                                                            BLE_UUID_SPS_WRITE_CHARACTERISTICS); 
/// Sps read characteristic
const struct att_char_desc sps_read_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                            0,
                                                            BLE_UUID_SPS_READ_CHARACTERISTICS); 
/// Sps indication characteristic
const struct att_char_desc sps_ntf_char = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                            0,
                                                            BLE_UUID_SPS_NOTIFY_CHARACTERISTICS);

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* sps_prf_itf_get(void)
{
    return &sps_itf;
}

void sps_send_write_val(uint8_t* val, uint32_t length)
{
	struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
    // Allocate the write value indication message
   struct sps_val_write_ind *ind = KE_MSG_ALLOC(SPS_VAL_WRITE_IND,
                                              prf_dst_task_get(&(sps_env->prf_env), sps_env->cursor), 
                                              prf_src_task_get(&(sps_env->prf_env), sps_env->cursor),
                                              sps_val_write_ind);
   // Fill in the parameter structure
   ind->conhdl = gapc_get_conhdl(sps_env->cursor);
   memcpy(ind->value, val, length);
   ind->length = length;
   
   // Send the message
   ke_msg_send(ind);
}

void sps_enable_indication(bool isEnable)
{
		struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
    // Allocate the enable indication request message
   struct sps_enable_indication_req *ind = KE_MSG_ALLOC(SPS_ENABLE_IND_REQ,
                                              prf_dst_task_get(&(sps_env->prf_env), sps_env->cursor), 
                                              prf_src_task_get(&(sps_env->prf_env), sps_env->cursor),
                                              sps_enable_indication_req);
   // Fill in the parameter structure
   ind->conhdl = gapc_get_conhdl(sps_env->cursor);
   ind->isEnable = isEnable;
   
   // Send the message
   ke_msg_send(ind);
}

void sps_indication_cfm_send(uint8_t status)
{
		struct sps_env_tag *sps_env = PRF_ENV_GET(SPS, sps);
	// allocate indication confirmation message to tell application layer the host has received the indication sent from device
    struct sps_indication_cfm *cfm = KE_MSG_ALLOC(SPS_SEND_INDICATION_CFM,
                                                 prf_dst_task_get(&(sps_env->prf_env), sps_env->cursor), 
																								 prf_src_task_get(&(sps_env->prf_env), sps_env->cursor),
                                                 sps_indication_cfm);
    cfm->status = status;
    
    // Send the message
    ke_msg_send(cfm);
}

#endif // (BLE_SPS_SERVER)
