/**
 ****************************************************************************************
 *
 * @file mi_attm_db_128.h
 *
 * @brief Header file of service database of 128bits long UUID.
 *
 * Copyright (C) 2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com>
 *
 ****************************************************************************************
 */

#ifndef __MI_ATTM_DB_128_H
#define __MI_ATTM_DB_128_H

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "ke_task.h"
#include "att.h"



/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

															 
uint8_t attm_svc_create_db_mijia(uint16_t *shdl, uint8_t *cfg_flag, uint8_t max_nb_att,
                               uint8_t *att_tbl, ke_task_id_t const dest_id,
                               const struct attm_desc_128 *att_db, uint8_t svc_perm);

#endif // __ATTM_DB_128_H
