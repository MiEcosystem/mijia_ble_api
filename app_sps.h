/**
 ****************************************************************************************
 *
 * @file app_customs.h
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

#ifndef _APP_SPS_H_
#define _APP_SPS_H_

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
void app_sps_init(void);

/**
 ****************************************************************************************
 * @brief Add a Custom Service instance to the DB.
 * @return void
 ****************************************************************************************
 */
void app_sps_create_db(void);

/**
 ****************************************************************************************
 * @brief Enable the Custom Service.
 * @param[in] conhdl    Connection handle
 * @return void
 ****************************************************************************************
 */
void app_sps_enable(uint16_t conhdl);

/*
 * Other Sps app functions
 ****************************************************************************************
 */
void ble_sps_process_received_data(uint8_t* pData, uint32_t length);

void ble_sps_send_data(uint8_t* pData,uint8_t length);

#endif // (BLE_CUSTOM_SERVER)

#endif // _APP_CUSTOMS_H_
