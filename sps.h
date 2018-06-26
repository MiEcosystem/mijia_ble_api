/**
 ****************************************************************************************
 *
 * @file sps.h
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

#ifndef _SPS_H_
#define _SPS_H_

/// Device Information Service Server Role
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif
#if !defined (BLE_CUSTOM_SERVER)
    #define BLE_CUSTOM_SERVER       1
#endif

#if (BLE_SPS_SERVER)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "prf_types.h"
#include "prf.h"
#include "attm_db_128.h"

#include "custom_common.h"

#define SPS_IDX_MAX        (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

// should following sps airsync protocol
#define	COMPANY_IDENTIFIER						    0x0056 
 
#define BLE_UUID_SPS_SERVICE									{0xE0,0xFE}
#define BLE_UUID_SPS_WRITE_CHARACTERISTICS 		(0xFEE2)
#define BLE_UUID_SPS_NOTIFY_CHARACTERISTICS 	(0xFEE1)
#define BLE_UUID_SPS_READ_CHARACTERISTICS 		(0xFEC9)

#define BLE_SPS_MAX_DATA_LEN            (ATT_DEFAULT_MTU - 3)


enum
{
    SPS_IDX_SVC,

    SPS_IDX_WRITE_CHAR,
    SPS_IDX_WRITE_VAL,
    
	SPS_IDX_IND_CHAR,
	SPS_IDX_IND_VAL,
	SPS_IDX_IND_CFG,

    SPS_IDX_READ_CHAR,
    SPS_IDX_READ_VAL,
    
    SPS_IDX_NB,

};

/// Value element
struct sps_val_elmt
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
struct sps_env_tag
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
    /// SPS task state
    ke_state_t state[SPS_IDX_MAX];};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// SPS Service - write Characteristic
extern const struct att_char_desc sps_write_char;
/// SPS Service - read Characteristic
extern const struct att_char_desc sps_read_char;
/// SPS Service - indication Characteristic
extern const struct att_char_desc sps_ntf_char;


extern struct attm_desc_128 sps_att_db[SPS_IDX_NB];

		
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
void sps_disable(uint16_t conhdl);

const struct prf_task_cbs *sps_prf_itf_get(void);

void sps_send_write_val(uint8_t* val, uint32_t length);

void sps_enable_indication(bool isEnable);

void sps_indication_cfm_send(uint8_t status);





#endif // (BLE_SPS_SERVER)

#endif // _SPS_H_
