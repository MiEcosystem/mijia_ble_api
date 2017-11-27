/**
 ****************************************************************************************
 *
 * @file mijia.h
 *
 * @brief Custom Service profile header file.
 *
 * Copyright (C) 2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _MIJIA_H_
#define _MIJIA_H_

/// Device Information Service Server Role
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif
#if !defined (BLE_CUSTOM_SERVER)
    #define BLE_CUSTOM_SERVER       1
#endif

#if (BLE_MIJIA_SERVER)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "prf_types.h"
#include "prf.h"
#include "attm_db_128.h"

#include "custom_common.h"
#include "mible_type.h"

#define MIJIA_IDX_MAX        (1)


#define MIJIA_IDX_ATT_DB_MAX   40  

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

// should following mijia airsync protocol
#define	COMPANY_IDENTIFIER						    0x0056 
 

#define BLE_MIJIA_MAX_DATA_LEN            (ATT_DEFAULT_MTU - 3)



/// Value element
struct mijia_val_elmt
{
    /// list element header
    struct co_list_hdr hdr;
    /// value identifier
    uint8_t att_idx;
    /// value length
    uint8_t length;
    /// value data
    uint8_t data[__ARRAY_EMPTY];
};

/// custs environment variable
struct mijia_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// Service Start Handle
    uint16_t shdl;
    /// To store the DB max number of attributes
    uint8_t max_nb_att;
    /// On-going operation
    struct ke_msg *operation;
	//Notification property status
    uint8_t feature;
    /// Cursor on connection used to notify peer devices
    uint8_t cursor;
    /// CCC handle index, used during notification/indication busy state
    uint8_t ccc_idx;

    /// List of values set by application
    struct co_list values;
    /// MIJIA task state
    ke_state_t state[MIJIA_IDX_MAX];};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct attm_desc_128 mijia_att_db[MIJIA_IDX_ATT_DB_MAX];

		
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 * @param[in] conhdl    Connection Handle
 ****************************************************************************************
 */
void mijia_disable(uint16_t conhdl);

const struct prf_task_cbs *mijia_prf_itf_get(void);

void mijia_send_write_val(mible_gatts_evt_t evt,uint16_t value_handle,uint8_t* val, uint32_t length);

void mijia_enable_indication_notification(uint16_t isEnable,uint16_t handle);

void mijia_ind_ntf_cfm_send(uint8_t status);


mible_status_t translate_miarch_attdb(mible_gatts_db_t * p_server_db);

mible_gatts_db_t *get_server_db(void);

bool get_wr_author(uint16_t value_handle);


#endif // (BLE_MIJIA_SERVER)

#endif // _MIJIA_H_
