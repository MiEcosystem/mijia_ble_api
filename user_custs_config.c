/**
 ****************************************************************************************
 *
 * @file user_custs_config.c
 *
 * @brief Custom1/2 Server (CUSTS1/2) profile database structure and initialization.
 *
 * Copyright (C) 2016 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup USER_CONFIG
 * @ingroup USER
 * @brief Custom1/2 Server (CUSTS1/2) profile database structure and initialization.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_prf_types.h"
//#include "app_customs.h"
#include "user_custs1_def.h"
#if (BLE_DWS_SERVER)
#include "user_dws_config.h"
#include "user_dws.h"
#endif
#if (BLE_SPS_SERVER)
#include "user_sps_config.h"
#include "user_spss.h"
#endif
#if (BLE_MIJIA_SERVER)
#include "mijia.h"
#include "app_mijia.h"
#endif


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Custom1/2 server function callback table
const struct cust_prf_func_callbacks cust_prf_funcs[] =
{
#if (BLE_CUSTOM1_SERVER)
    {   TASK_ID_CUSTS1,
        custs1_att_db,
        CUST1_IDX_NB,
        #if (BLE_APP_PRESENT)
        app_custs1_create_db, app_custs1_enable,
        #else
        NULL, NULL,
        #endif
        custs1_init, NULL
    },
#endif
#if (BLE_CUSTOM2_SERVER)
    {   TASK_ID_CUSTS2,
        NULL,
        0,
        #if (BLE_APP_PRESENT)
        app_custs2_create_db, app_custs2_enable,
        #else
        NULL, NULL,
        #endif
        custs2_init, NULL
    },
#endif
#if (BLE_DWS_SERVER)
    {
        TASK_ID_DWS,
        dws_att_db,
        DWS_IDX_NB,
        #if (BLE_APP_PRESENT)
        app_dws_create_db, app_dws_enable ,
        #else
        NULL, NULL,
        #endif
        dws_init, NULL
    },
#endif   

#if (BLE_MIJIA_SERVER)
    {   TASK_ID_MIJIA,
        mijia_att_db,
        MIJIA_IDX_ATT_DB_MAX,		//¡Ÿ ±÷µ  
        #if (BLE_APP_PRESENT)
        app_mijia_create_db, /*app_mijia_enable*/ NULL,
        #else
        NULL, NULL,
        #endif
        NULL, NULL
    },
#endif
		
//#if (BLE_SPS_SERVER)
//    {
//        TASK_ID_SPS_SERVER,
//        sps_att_db,
//        SPSS_IDX_NB,
//        #if (BLE_APP_PRESENT)
//        user_sps_create_db, user_sps_enable ,
//        #else
//        NULL, NULL,
//        #endif
//        NULL, NULL
//    },
//#endif   

		
    {TASK_ID_INVALID, NULL, 0, NULL, NULL, NULL, NULL},   // DO NOT MOVE. Must always be last
};

/// @} USER_CONFIG
