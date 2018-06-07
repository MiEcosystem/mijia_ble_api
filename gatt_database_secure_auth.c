/*
 * gatt_database_secure_auth.c
 *
 *  Created on: 2018年3月6日
 *      Author: JT
 */
#include <stdbool.h>
#include "gatt_database.h"

// All characteristics value

static uint8_t version[20] = "1.1.0_0000";
static uint8_t opcode[4];
static uint8_t security_auth[20];
static uint8_t ota_control[20];
static uint8_t ota_data[20];

/* GATT database derived from Silicon labs' GATT database*/
mible_gatts_char_db_t gattCharacterisitcs[] = {

    // Version
    [0] = {
            .char_uuid = { 0, { 0x0004 } },
            .char_property = MIBLE_READ | MIBLE_NOTIFY,
            .p_value = version,
            .char_value_len = sizeof(version),
            .char_value_handle = gattdb_version,
            .is_variable_len = false,
            .rd_author = false,
            .wr_author = false,
            .char_desc_db = {0}
    },

    // Auth Control Point
    [1] = {
            .char_uuid = { 0, { 0x0010 } },
            .char_property = MIBLE_WRITE_WITHOUT_RESP | MIBLE_NOTIFY,
            .p_value = opcode,
            .char_value_len = sizeof(opcode),
            .char_value_handle = gattdb_auth,
            .is_variable_len = false,
            .rd_author = false,
            .wr_author = false,
            .char_desc_db = {0}
    },

    // Secure auth
    [2] = {
            .char_uuid = { 0, { 0x0016 } },
            .char_property = MIBLE_WRITE_WITHOUT_RESP | MIBLE_NOTIFY,
            .p_value = security_auth,
            .char_value_len = sizeof(security_auth),
            .char_value_handle = gattdb_secure_auth,
            .is_variable_len = false,
            .rd_author = false,
            .wr_author = false,
            .char_desc_db = {0}
    },

    // OTA control point
    [3] = {
            .char_uuid = { 0, { 0x0017 } },
            .char_property = MIBLE_WRITE | MIBLE_NOTIFY,
            .p_value = ota_control,
            .char_value_len = sizeof(ota_control),
            .char_value_handle = gattdb_ota_control,
            .is_variable_len = 1,
    },

    // OTA data
    [4] = {
            .char_uuid = { 0, { 0x0018 } },
            .char_property = MIBLE_WRITE_WITHOUT_RESP | MIBLE_NOTIFY,
            .p_value = ota_data,
            .char_value_len = sizeof(ota_data),
            .char_value_handle = gattdb_ota_data,
            .is_variable_len = 1,
    }
};

gatt_database_t gatt_database = {
        sizeof(gattCharacterisitcs) / sizeof(gattCharacterisitcs[0]), gattCharacterisitcs
};
