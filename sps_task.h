/**
 ****************************************************************************************
 *
 * @file sps_task.h
 *
 * @brief Custom Service profile task header file.
 *
 * Copyright (C) 2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef __SPS_TASK_PRF_H
#define __SPS_TASK_PRF_H

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SPS_SERVER)

#include <stdint.h>
#include "ke_task.h"
#include "prf_types.h"
#include "compiler.h"        // compiler definition
#include "att.h"
#include "attm_db_128.h"
#include "sps.h"

/**** NOTIFICATION BITS ****/
extern uint8_t FirstNotificationBit;
extern uint8_t SecondNotificationBit;



/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the CUSTOMS task
enum sps_state
{
    /// Idle state
    SPS_IDLE,
    /// Busy state
    SPS_BUSY,
    /// Number of defined states.
    SPS_STATE_MAX,
};

/// Messages for SPS
enum
{
    /// Add a CUSTOMS instance into the database
    SPS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_ID_SPS),
    /// Inform APP of database creation status
    SPS_CREATE_DB_CFM,
    /// Start the Custom Service Task - at connection
    SPS_ENABLE_REQ,
    /// Indicate that the characteristic value has been written
    SPS_VAL_WRITE_IND,
    /// Inform the application that the profile service role task has been disabled after a disconnection
    SPS_DISABLE_IND,
    /// enable indication
    SPS_ENABLE_IND_REQ,
    ///Send data to peer master
    SPS_SEND_REQ,
    ///Inform the handling result of indication result
    SPS_SEND_INDICATION_CFM,
    
    SPS_SEND_DATA_TO_MASTER,
    /// Profile error report
    SPS_ERROR_IND,
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters for the database creation
struct sps_db_cfg
{
    ///max number of casts1 service characteristics
    uint8_t max_nb_att;
    uint8_t *att_tbl;
    uint8_t *cfg_flag;

    ///Database configuration
    uint16_t features;
};

/// Parameters of the @ref SPS_CREATE_DB_CFM message
struct sps_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// Parameters of the @ref SPS_ENABLE_REQ message
struct sps_enable_req
{
    /// Connection Handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;    
    /// SPS_IDX_IND_VAL property status
    uint8_t feature;
};

/// Parameters of the @ref SPS_DISABLE_IND message
struct sps_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
};

/// Parameters of the @ref SPS_VAL_WRITE_IND massage
struct sps_val_write_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// data length
    uint16_t length;
    /// Value
    uint8_t value[BLE_SPS_MAX_DATA_LEN];
};

/// Parameters of the @ref SPS_ENABLE_IND_REQ message
struct sps_enable_indication_req
{
    /// Connection handle
    uint16_t conhdl;
    /// true if enable, false if disable
    uint16_t isEnable;
    
};

/// Parameters of the @ref SPS_REQ message
struct sps_indication_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Indicated data length
    uint16_t length;
    /// Characteristic Value
    uint8_t value[BLE_SPS_MAX_DATA_LEN];
    
};

/// Parameters of the @ref SPS_SEND_INDICATION_CFM message
struct sps_indication_cfm
{
    /// Status
    uint8_t status;
};

struct sps_send_data_req
{
    uint32_t dataLen;
    uint8_t* pDataBuf;
};


/**
 ****************************************************************************************
 * @brief Initialize Client Characteristic Configuration fields.
 * @details Function initializes all CCC fields to default value.
 * @param[in] att_db         Id of the message received.
 * @param[in] max_nb_att     Pointer to the parameters of the message.
 ****************************************************************************************
 */
void sps_init_ccc_values(const struct attm_desc_128 *att_db, int max_nb_att);

/**
 ****************************************************************************************
 * @brief Set per connection CCC value for attribute
 * @details Function sets CCC for specified connection.
 * @param[in] conidx         Connection index.
 * @param[in] att_idx        Attribute index.
 * @param[in] ccc            Value of ccc.
 ****************************************************************************************
 */
void sps_set_ccc_value(uint8_t conidx, uint8_t att_idx, uint16_t ccc);

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler sps_state_handler[SPS_STATE_MAX];
extern const struct ke_state_handler sps_default_handler;

#endif // BLE_SPS_SERVER
#endif // __SPS_TASK_PRF_H
