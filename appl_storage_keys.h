/**
 ****************************************************************************************
 *
 * @file appl_storage_keys.h
 *
 * @brief HAP storage keys
 *
 * Copyright (C) 2015-2018 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef APPL_STORAGE_KEYS_H_
#define APPL_STORAGE_KEYS_H_

#define APPL_STORAGE_CAT(category)       (category << 24)

enum appl_storage_category {
        APPL_STORAGE_MESH_LIBRARY_CAT_RSV0 = 0x00,
        APPL_STORAGE_MESH_LIBRARY_CAT_RSV1 = 0x01,
        APPL_STORAGE_MESH_LIBRARY_CAT_RSV2 = 0x02,
        APPL_STORAGE_MESH_LIBRARY_CAT_RSV3 = 0x03,
        APPL_STORAGE_MESH_LIBRARY_CAT_RSV4 = 0x04,
        APPL_STORAGE_MESH_LIBRARY_CAT_RSV5 = 0x05,
        APPL_STORAGE_MESH_LIBRARY_CAT_RSV6 = 0x06,
        APPL_STORAGE_USER_DATA_CAT = 0xFF,
};

enum storage {
        STORAGE_MS_PS_RECORD_BD_ADDR = APPL_STORAGE_CAT(APPL_STORAGE_USER_DATA_CAT),
        STORAGE_MS_PS_RECORD_AVAILABLE_UNICAST_ADDR,
        STORAGE_MS_PS_RECORD_BEARER,
        STORAGE_MS_PS_RECORD_SUOTA,
        STORAGE_MS_PS_RECORD_AUTH_METHOD,
        STORAGE_MS_PS_RECORD_INPUT_OOB_ACTION_CAP,
        STORAGE_MS_PS_RECORD_OUTPUT_OOB_ACTION_CAP,
        STORAGE_MS_PS_RECORD_INPUT_OOB_ACTION_PROV_START,
        STORAGE_MS_PS_RECORD_OUTPUT_OOB_ACTION_PROV_START,

};

typedef uint32_t storage_key_t;

#endif /* APPL_STORAGE_KEYS_H_ */
