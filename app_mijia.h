/**
 ****************************************************************************************
 *
 * @file app_mijia.h
 *
 * @brief Custom Service Application entry point.
 *
 * Copyright (C) 2015-2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _APP_MIJIA_H_
#define _APP_MIJIA_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "user_profiles_config.h"

#if (BLE_CUSTOM_SERVER)

#include <stdint.h>
#include "prf_types.h"
#include "attm_db_128.h"


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Custom Service Application.
 * @return void
 ****************************************************************************************
 */
void app_mijia_init(void);

/**
 ****************************************************************************************
 * @brief Add a Custom Service instance to the DB.
 * @return void
 ****************************************************************************************
 */
void app_mijia_create_db(void);

/**
 ****************************************************************************************
 * @brief Enable the Custom Service.
 * @param[in] conhdl    Connection handle
 * @return void
 ****************************************************************************************
 */
void app_mijia_enable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief It is called to validate the characteristic value before it is written
 *        to profile database.
 * @param[in] att_idx Attribute index to be validated before it is written to database.
 * @param[in] offset  Offset at which the data has to be written (used only for values
 *                    that are greater than MTU).
 * @param[in] len     Data length to be written.
 * @param[in] value   Pointer to data to be written to attribute database.
 * @return validation status (high layer error code)
 ****************************************************************************************
 */
uint8_t app_mijia_val_wr_validate(uint16_t att_idx, uint16_t offset, uint16_t len, uint8_t *value);


void ble_mijia_send_data(uint16_t handle,uint8_t* pData, uint32_t length);

/*
 * Other Mijia app functions
 ****************************************************************************************
 */


#endif // (BLE_CUSTOM_SERVER)

#endif // _APP_CUSTOMS_H_
