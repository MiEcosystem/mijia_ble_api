#ifndef __MIBLE_MESH_API_H
#define __MIBLE_MESH_API_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __packed
#define __packed __attribute__((packed))
#endif

#include "mijia_ble_api/mible_type.h"

/*SIG Generic model ID */
#define MIBLE_MESH_MODEL_ID_CONFIGURATION_SERVER                    0x0000
#define MIBLE_MESH_MODEL_ID_CONFIGURATION_CLIENT                    0x0001
#define MIBLE_MESH_MODEL_ID_HEALTH_SERVER                           0x0002
#define MIBLE_MESH_MODEL_ID_HEALTH_CLIENT                           0x0003
#define MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER                    0x1000
#define MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_CLIENT                    0x1001
#define MIBLE_MESH_MODEL_ID_GENERIC_LEVEL_SERVER                    0x1002
#define MIBLE_MESH_MODEL_ID_GENERIC_LEVEL_CLIENT                    0x1003
#define MIBLE_MESH_MODEL_ID_GENERIC_DEFAULT_TRANSITION_TIME_SERVER  0x1004
#define MIBLE_MESH_MODEL_ID_GENERIC_DEFAULT_TRANSITION_TIME_CLIENT  0x1005
#define MIBLE_MESH_MODEL_ID_GENERIC_POWER_ONOFF_SERVER              0x1006
#define MIBLE_MESH_MODEL_ID_GENERIC_POWER_ONOFF_SETUP_SERVER        0x1007
#define MIBLE_MESH_MODEL_ID_GENERIC_POWER_ONOFF_CLIENT              0x1008
#define MIBLE_MESH_MODEL_ID_GENERIC_POWER_LEVEL_SERVER              0x1009
#define MIBLE_MESH_MODEL_ID_GENERIC_POWER_LEVEL_SETUP_SERVER        0x100a
#define MIBLE_MESH_MODEL_ID_GENERIC_POWER_LEVEL_CLENT               0x100b
#define MIBLE_MESH_MODEL_ID_GENERIC_BATTERY_SERVER                  0x100c
#define MIBLE_MESH_MODEL_ID_GENERIC_BATTERY_CLIENT                  0x100d
#define MIBLE_MESH_MODEL_ID_GENERIC_LOCATION_SERVER                 0x100e
#define MIBLE_MESH_MODEL_ID_GENERIC_LOCATION_SETUP_SERVER           0x100f
#define MIBLE_MESH_MODEL_ID_GENERIC_LOCATION_CLIENT                 0x1010
#define MIBLE_MESH_MODEL_ID_GENERIC_ADMIN_PROPERTY_SERVER           0x1011
#define MIBLE_MESH_MODEL_ID_GENERIC_MANUFACTURER_PROPERTY_SERVER    0x1012
#define MIBLE_MESH_MODEL_ID_GENERIC_USER_PROPERTY_SERVER            0x1013
#define MIBLE_MESH_MODEL_ID_GENERIC_CLIENT_PROPERTY_SERVER          0x1014
#define MIBLE_MESH_MODEL_ID_GENERIC_PROPERTY_CLENT                  0x1015

#define MIBLE_MESH_MODEL_ID_LIGHTNESS_SERVER                        0x1300
#define MIBLE_MESH_MODEL_ID_LIGHTNESS_SETUP_SERVER                  0x1301
#define MIBLE_MESH_MODEL_ID_LIGHTNESS_CLIENT                        0x1302
#define MIBLE_MESH_MODEL_ID_CTL_SERVER                              0x1303
#define MIBLE_MESH_MODEL_ID_CTL_SETUP_SERVER                        0x1304
#define MIBLE_MESH_MODEL_ID_CTL_CLIENT                              0x1305
#define MIBLE_MESH_MODEL_ID_CTL_TEMPEATURE_SERVER                   0x1306
#define MIBLE_MESH_MODEL_ID_HSL_SERVER                              0x1307
#define MIBLE_MESH_MODEL_ID_HSL_SETUP_SERVER                        0x1308
#define MIBLE_MESH_MODEL_ID_HSL_CLIENT                              0x1309

/******** Mesh Opcodes **********/
#define MIBLE_MESH_MSG_CONFIG_BEACON_GET                            0x8009
#define MIBLE_MESH_MSG_CONFIG_BEACON_SET                            0x800A
#define MIBLE_MESH_MSG_CONFIG_BEACON_STATUS                         0x800B
#define MIBLE_MESH_MSG_CONFIG_COMPOSITION_DATA_GET                  0x8008
#define MIBLE_MESH_MSG_CONFIG_COMPOSITION_DATA_STATUS               0x02
#define MIBLE_MESH_MSG_CONFIG_DEFAULT_TTL_GET                       0x800C
#define MIBLE_MESH_MSG_CONFIG_DEFAULT_TTL_SET                       0x800D
#define MIBLE_MESH_MSG_CONFIG_DEFAULT_TTL_STATUS                    0x800E
#define MIBLE_MESH_MSG_CONFIG_GATT_PROXY_GET                        0x8012
#define MIBLE_MESH_MSG_CONFIG_GATT_PROXY_SET                        0x8013
#define MIBLE_MESH_MSG_CONFIG_GATT_PROXY_STATUS                     0x8014
#define MIBLE_MESH_MSG_CONFIG_FRIEND_GET                            0x800F
#define MIBLE_MESH_MSG_CONFIG_FRIEND_SET                            0x8010
#define MIBLE_MESH_MSG_CONFIG_FRIEND_STATUS                         0x8011
#define MIBLE_MESH_MSG_CONFIG_MODEL_PUBLICATION_GET                 0x8018
#define MIBLE_MESH_MSG_CONFIG_MODEL_PUBLICATION_SET                 0x03
#define MIBLE_MESH_MSG_CONFIG_MODEL_PUBLICATION_STATUS              0x8019
#define MIBLE_MESH_MSG_CONFIG_MODEL_PUBLICATION_VIRTUAL_ADDRESS_SET 0x801A
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_ADD                0x801B
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE             0x801C
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE_ALL         0x801D
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_OVERWRITE          0x801E
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_STATUS             0x801F
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_VIRTUAL_ADDRESS_ADD 0x8020
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_VIRTUAL_ADDRESS_DELETE  0x8021
#define MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_VIRTUAL_ADDRESS_OVERWRITE   0x8022
#define MIBLE_MESH_MSG_CONFIG_NETWORK_TRANSMIT_GET                  0x8023
#define MIBLE_MESH_MSG_CONFIG_NETWORK_TRANSMIT_SET                  0x8024
#define MIBLE_MESH_MSG_CONFIG_NETWORK_TRANSMIT_STATUS               0x8025
#define MIBLE_MESH_MSG_CONFIG_RELAY_GET                             0x8026
#define MIBLE_MESH_MSG_CONFIG_RELAY_SET                             0x8027
#define MIBLE_MESH_MSG_CONFIG_RELAY_STATUS                          0x8028
#define MIBLE_MESH_MSG_CONFIG_SIG_MODEL_SUBSCRIPTION_GET            0x8029
#define MIBLE_MESH_MSG_CONFIG_SIG_MODEL_SUBSCRIPTION_LIST           0x802A
#define MIBLE_MESH_MSG_CONFIG_VENDOR_MODEL_SUBSCRIPTION_GET         0x802B
#define MIBLE_MESH_MSG_CONFIG_VENDOR_MODEL_SUBSCRIPTION_LIST        0x802C
#define MIBLE_MESH_MSG_CONFIG_LOW_POWER_NODE_POLL_TIMEOUT_GET       0x802D
#define MIBLE_MESH_MSG_CONFIG_LOW_POWER_NODE_POLL_TIMEOUT_STATUS    0x802E
#define MIBLE_MESH_MSG_CONFIG_NETKEY_ADD                            0x8040
#define MIBLE_MESH_MSG_CONFIG_NETKEY_DELETE                         0x8041
#define MIBLE_MESH_MSG_CONFIG_NETKEY_GET                            0x8042
#define MIBLE_MESH_MSG_CONFIG_NETKEY_LIST                           0x8043
#define MIBLE_MESH_MSG_CONFIG_NETKEY_STATUS                         0x8044
#define MIBLE_MESH_MSG_CONFIG_NETKEY_UPDATE                         0x8045
#define MIBLE_MESH_MSG_CONFIG_APPKEY_ADD                            0x00
#define MIBLE_MESH_MSG_CONFIG_APPKEY_UPDATE                         0x01
#define MIBLE_MESH_MSG_CONFIG_APPKEY_DELETE                         0x8000
#define MIBLE_MESH_MSG_CONFIG_APPKEY_GET                            0x8001
#define MIBLE_MESH_MSG_CONFIG_APPKEY_LIST                           0x8002
#define MIBLE_MESH_MSG_CONFIG_APPKEY_STATUS                         0x8003
#define MIBLE_MESH_MSG_CONFIG_MODEL_APP_BIND                        0x803D
#define MIBLE_MESH_MSG_CONFIG_MODEL_APP_STATUS                      0x803E
#define MIBLE_MESH_MSG_CONFIG_MODEL_APP_UNBIND                      0x803F
#define MIBLE_MESH_MSG_CONFIG_SIG_MODEL_APP_GET                     0x804B
#define MIBLE_MESH_MSG_CONFIG_SIG_MODEL_APP_LIST                    0x804C
#define MIBLE_MESH_MSG_CONFIG_VENDOR_MODEL_APP_GET                  0x804D
#define MIBLE_MESH_MSG_CONFIG_VENDOR_MODEL_APP_LIST                 0x804E
#define MIBLE_MESH_MSG_CONFIG_NODE_IDENTITY_GET                     0x8046
#define MIBLE_MESH_MSG_CONFIG_NODE_IDENTITY_SET                     0x8047
#define MIBLE_MESH_MSG_CONFIG_NODE_IDENTITY_STATUS                  0x8048
#define MIBLE_MESH_MSG_CONFIG_NODE_RESET                            0x8049
#define MIBLE_MESH_MSG_CONFIG_NODE_RESET_STATUS                     0x804A
#define MIBLE_MESH_MSG_CONFIG_KEY_REFRESH_PHASE_GET                 0x8015
#define MIBLE_MESH_MSG_CONFIG_KEY_REFRESH_PHASE_SET                 0x8016
#define MIBLE_MESH_MSG_CONFIG_KEY_REFRESH_PHASE_STATUS              0x8017
#define MIBLE_MESH_MSG_CONFIG_HEARTBEAT_PUBLICATION_GET             0x8038
#define MIBLE_MESH_MSG_CONFIG_HEARTBEAT_PUBLICATION_SET             0x8039
#define MIBLE_MESH_MSG_CONFIG_HEARTBEAT_PUBLICATION_STATUS          0x06
#define MIBLE_MESH_MSG_CONFIG_HEARTBEAT_SUBSCRIPTION_GET            0x803A
#define MIBLE_MESH_MSG_CONFIG_HEARTBEAT_SUBSCRIPTION_SET            0x803B
#define MIBLE_MESH_MSG_CONFIG_HEARTBEAT_SUBSCRIPTION_STATUS         0x803C

#define MIBLE_MESH_MSG_HEALTH_CURRENT_STATUS                        0x04
#define MIBLE_MESH_MSG_HEALTH_FAULT_STATUS                          0x05
#define MIBLE_MESH_MSG_HEALTH_FAULT_CLEAR                           0x802F
#define MIBLE_MESH_MSG_HEALTH_FAULT_CLEAR_UNACKNOWLEDGED            0x8030
#define MIBLE_MESH_MSG_HEALTH_FAULT_GET                             0x8031
#define MIBLE_MESH_MSG_HEALTH_FAULT_TEST                            0x8032
#define MIBLE_MESH_MSG_HEALTH_FAULT_TEST_UNACKNOWLEDGED             0x8033
#define MIBLE_MESH_MSG_HEALTH_PERIOD_GET                            0x8034
#define MIBLE_MESH_MSG_HEALTH_PERIOD_SET                            0x8035
#define MIBLE_MESH_MSG_HEALTH_PERIOD_SET_UNACKNOWLEDGED             0x8036
#define MIBLE_MESH_MSG_HEALTH_PERIOD_STATUS                         0x8037
#define MIBLE_MESH_MSG_HEALTH_ATTENTION_GET                         0x8004
#define MIBLE_MESH_MSG_HEALTH_ATTENTION_SET                         0x8005
#define MIBLE_MESH_MSG_HEALTH_ATTENTION_SET_UNACKNOWLEDGED          0x8006
#define MIBLE_MESH_MSG_HEALTH_ATTENTION_STATUS                      0x8007

/*Generic On Off Model Message Definition*/
#define MIBLE_MESH_MSG_GENERIC_ONOFF_GET                            0x8201
#define MIBLE_MESH_MSG_GENERIC_ONOFF_SET                            0x8202
#define MIBLE_MESH_MSG_GENERIC_ONOFF_SET_UNACKNOWLEDGED             0x8203
#define MIBLE_MESH_MSG_GENERIC_ONOFF_STATUS                         0x8204

/*Generic Level Model Message Definition*/
#define MIBLE_MESH_MSG_GENERIC_LEVEL_GET                                 0x8205
#define MIBLE_MESH_MSG_GENERIC_LEVEL_SET                                 0x8206
#define MIBLE_MESH_MSG_GENERIC_LEVEL_SET_UNACKNOWLEDGED                  0x8207
#define MIBLE_MESH_MSG_GENERIC_LEVEL_STATUS                              0x8208
#define MIBLE_MESH_MSG_GENERIC_DELTA_SET                                 0x8209
#define MIBLE_MESH_MSG_GENERIC_DELTA_SET_UNACKNOWLEDGED                  0x820A
#define MIBLE_MESH_MSG_GENERIC_MOVE_SET                                  0x820B
#define MIBLE_MESH_MSG_GENERIC_MOVE_SET_UNACKNOWLEDGED                   0x820C

/*LIGHTNESS Model Message Definition*/
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_GET                               0x824B
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET                               0x824C
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET_UNACKNOWLEDGED                0x824D
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_STATUS                            0x824E
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_LINEAR_GET                        0x824F
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_LINEAR_SET                        0x8250
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_LINEAR_SET_UNACKNOWLEDGED         0x8251
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_LINEAR_STATUS                     0x8252
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_LAST_GET                          0x8253
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_LAST_STATUS                       0x8254
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_DEFAULT_GET                       0x8255
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_DEFAULT_STATUS                    0x8256
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_RANGE_GET                         0x8257
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_RANGE_STATUS                      0x8258
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_DEFAULT_SET                       0x8259
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_DEFAULT_SET_UNACKNOWLEDGED        0x825A
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_RANGE_SET                         0x825B
#define MIBLE_MESH_MSG_LIGHT_LIGHTNESS_RANGE_SET_UNACKNOWLEDGED          0x825C

/*CTL Model Message Definition*/
#define MIBLE_MESH_MSG_LIGHT_CTL_GET                                     0x825D
#define MIBLE_MESH_MSG_LIGHT_CTL_SET                                     0x825E
#define MIBLE_MESH_MSG_LIGHT_CTL_SET_UNACKNOWLEDGED                      0x825F
#define MIBLE_MESH_MSG_LIGHT_CTL_STATUS                                  0x8260
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_GET                         0x8261
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_RANGE_GET                   0x8262
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_RANGE_STATUS                0x8263
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET                         0x8264
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET_UNACKNOWLEDGED          0x8265
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_STATUS                      0x8266
#define MIBLE_MESH_MSG_LIGHT_CTL_DEFAULT_GET                             0x8267
#define MIBLE_MESH_MSG_LIGHT_CTL_DEFAULT_STATUS                          0x8268
#define MIBLE_MESH_MSG_LIGHT_CTL_DEFAULT_SET                             0x8269
#define MIBLE_MESH_MSG_LIGHT_CTL_DEFAULT_SET_UNACKNOWLEDGED              0x826A
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_RANGE_SET                   0x826B
#define MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_RANGE_SET_UNACKNOWLEDGED    0x826C

/*HSL Model Message Definition*/
#define MIBLE_MESH_MSG_LIGHT_HSL_GET                                     0x826D
#define MIBLE_MESH_MSG_LIGHT_HSL_HUE_GET                                 0x826E
#define MIBLE_MESH_MSG_LIGHT_HSL_HUE_SET                                 0x826F
#define MIBLE_MESH_MSG_LIGHT_HSL_HUE_SET_UNACKNOWLEDGED                  0x8270
#define MIBLE_MESH_MSG_LIGHT_HSL_HUE_STATUS                              0x8271
#define MIBLE_MESH_MSG_LIGHT_HSL_SATURATION_GET                          0x8272
#define MIBLE_MESH_MSG_LIGHT_HSL_SATURATION_SET                          0x8273
#define MIBLE_MESH_MSG_LIGHT_HSL_SATURATION_SET_UNACKNOWLEDGED           0x8274
#define MIBLE_MESH_MSG_LIGHT_HSL_SATURATION_STATUS                       0x8275
#define MIBLE_MESH_MSG_LIGHT_HSL_SET                                     0x8276
#define MIBLE_MESH_MSG_LIGHT_HSL_SET_UNACKNOWLEDGED                      0x8277
#define MIBLE_MESH_MSG_LIGHT_HSL_STATUS                                  0x8278
#define MIBLE_MESH_MSG_LIGHT_HSL_TARGET_GET                              0x8279
#define MIBLE_MESH_MSG_LIGHT_HSL_TARGET_STATUS                           0x827A
#define MIBLE_MESH_MSG_LIGHT_HSL_DEFAULT_GET                             0x827B
#define MIBLE_MESH_MSG_LIGHT_HSL_DEFAULT_STATUS                          0x827C
#define MIBLE_MESH_MSG_LIGHT_HSL_RANGE_GET                               0x827D
#define MIBLE_MESH_MSG_LIGHT_HSL_RANGE_STATUS                            0x827E
#define MIBLE_MESH_MSG_LIGHT_HSL_DEFAULT_SET                             0x827F
#define MIBLE_MESH_MSG_LIGHT_HSL_DEFAULT_SET_UNACKNOWLEDGED              0x8280
#define MIBLE_MESH_MSG_LIGHT_HSL_RANGE_SET                               0x8281
#define MIBLE_MESH_MSG_LIGHT_HSL_RANGE_SET_UNACKNOWLEDGED                0x8282

/*XYL Model Opcodes Definition*/
#define MIBLE_MESH_MSG_LIGHT_XYL_GET                         0x8283
#define MIBLE_MESH_MSG_LIGHT_XYL_SET                         0x8284
#define MIBLE_MESH_MSG_LIGHT_XYL_SET_UNACK                   0x8285
#define MIBLE_MESH_MSG_LIGHT_XYL_STATUS                      0x8286
#define MIBLE_MESH_MSG_LIGHT_XYL_TARGET_GET                  0x8287
#define MIBLE_MESH_MSG_LIGHT_XYL_TARGET_STATUS               0x8288
#define MIBLE_MESH_MSG_LIGHT_XYL_DEFAULT_GET                 0x8289
#define MIBLE_MESH_MSG_LIGHT_XYL_DEFAULT_STATUS              0x828A
#define MIBLE_MESH_MSG_LIGHT_XYL_RANGE_GET                   0x828B
#define MIBLE_MESH_MSG_LIGHT_XYL_RANGE_STATUS                0x828C
#define MIBLE_MESH_MSG_LIGHT_XYL_DEFAULT_SET                 0x828D
#define MIBLE_MESH_MSG_LIGHT_XYL_DEFAULT_SET_UNACK           0x828E
#define MIBLE_MESH_MSG_LIGHT_XYL_RANGE_SET                   0x828F
#define MIBLE_MESH_MSG_LIGHT_XYL_RANGE_SET_UNACK             0x8290

/*LC Model Opcodes Definition*/
#define MIBLE_MESH_MSG_LIGHT_LC_MODE_GET                     0x8291
#define MIBLE_MESH_MSG_LIGHT_LC_MODE_SET                     0x8292
#define MIBLE_MESH_MSG_LIGHT_LC_MODE_SET_UNACK               0x8293
#define MIBLE_MESH_MSG_LIGHT_LC_MODE_STATUS                  0x8294
#define MIBLE_MESH_MSG_LIGHT_LC_OM_GET                       0x8295
#define MIBLE_MESH_MSG_LIGHT_LC_OM_SET                       0x8296
#define MIBLE_MESH_MSG_LIGHT_LC_OM_SET_UNACK                 0x8297
#define MIBLE_MESH_MSG_LIGHT_LC_OM_STATUS                    0x8298
#define MIBLE_MESH_MSG_LIGHT_LC_ONOFF_GET                    0x8299
#define MIBLE_MESH_MSG_LIGHT_LC_ONOFF_SET                    0x829A
#define MIBLE_MESH_MSG_LIGHT_LC_ONOFF_SET_UNACK              0x829B
#define MIBLE_MESH_MSG_LIGHT_LC_ONOFF_STATUS                 0x829C
#define MIBLE_MESH_MSG_LIGHT_LC_PROPERTY_GET                 0x829D
#define MIBLE_MESH_MSG_LIGHT_LC_PROPERTY_SET                 0x62
#define MIBLE_MESH_MSG_LIGHT_LC_PROPERTY_SET_UNACK           0x63
#define MIBLE_MESH_MSG_LIGHT_LC_PROPERTY_STATUS              0x64

/*Sensor Model Opcodes Definition*/
#define MIBLE_MESH_MSG_SENSOR_DESCRIPTOR_GET                      0x8230
#define MIBLE_MESH_MSG_SENSOR_DESCRIPTOR_STATUS                   0x51
#define MIBLE_MESH_MSG_SENSOR_GET                                 0x8231
#define MIBLE_MESH_MSG_SENSOR_STATUS                              0x52
#define MIBLE_MESH_MSG_SENSOR_COLUMN_GET                          0x8232
#define MIBLE_MESH_MSG_SENSOR_COLUMN_STATUS                       0x53
#define MIBLE_MESH_MSG_SENSOR_SERIES_GET                          0x8233
#define MIBLE_MESH_MSG_SENSOR_SERIES_STATUS                       0x54
#define MIBLE_MESH_MSG_SENSOR_CADENCE_GET                         0x8234
#define MIBLE_MESH_MSG_SENSOR_CADENCE_SET                         0x55
#define MIBLE_MESH_MSG_SENSOR_CADENCE_SET_UNACKED                 0x56
#define MIBLE_MESH_MSG_SENSOR_CADENCE_STATUS                      0x57
#define MIBLE_MESH_MSG_SENSOR_SETTINGS_GET                        0x8235
#define MIBLE_MESH_MSG_SENSOR_SETTINGS_STATUS                     0x58
#define MIBLE_MESH_MSG_SENSOR_SETTING_GET                         0x8236
#define MIBLE_MESH_MSG_SENSOR_SETTING_SET                         0x59
#define MIBLE_MESH_MSG_SENSOR_SETTING_SET_UNACKED                 0x5A
#define MIBLE_MESH_MSG_SENSOR_SETTING_STATUS                      0x5B

/** Mesh vendor models **/
#define MIBLE_MESH_MIOT_SPEC_SERVER_MODEL                           0000 //0x038f0000
#define MIBLE_MESH_MIOT_SPEC_CLIENT_MODEL                           0001 //0x038f0001
#define MIBLE_MESH_MIJIA_SERVER_MODEL                               0002 //0x038f0002
#define MIBLE_MESH_MIJIA_CLIENT_MODEL                               0003 //0x038f0003

/*** Mesh vendor opcodes ***/
#define MIBLE_MESH_MIOT_SPEC_GET                                    0x00C1 //0xC1038F
#define MIBLE_MESH_MIOT_SPEC_SET                                    0x00C3 //0xC3038F
#define MIBLE_MESH_MIOT_SPEC_SET_NOACK                              0x00C4 //0xC4038F
#define MIBLE_MESH_MIOT_SPEC_STATUS                                 0x00C5 //0xC5038F

#define MIBLE_MESH_MIOT_SPEC_ACTION                                 0x00C6 //0xC6038F
#define MIBLE_MESH_MIOT_SPEC_ACTION_ACK                             0x00C7 //0xC7038F
#define MIBLE_MESH_MIOT_SPEC_NOTIFICATION                           0x00C8 //0xC8038F

#define MIBLE_MESH_MIOT_SPEC_INDICATION                             0x00CE //0xCE038F
#define MIBLE_MESH_MIOT_SPEC_INDICATION_ACK                         0x00CF //0xCF038F

#define MIBLE_MESH_MIOT_SPEC_VENDOR_CONFIG                          0x00FF //0xFF038F
#define MIBLE_MESH_MIOT_SPEC_VENDOR_CONFIG_SUB                      0x01

#define MIBLE_MESH_COMPANY_ID_SIG                                   0xFFFF
#define MIBLE_MESH_COMPANY_ID_XIAOMI                                0x038F

#define MIBLE_MESH_ADV_PKT_INTERNAL                                 40 //48
#define MIBLE_MESH_ADV_PKT_COUNTER                                  3

#define MIBLE_MESH_DEV_UUID_LEN                                     16
#define MIBLE_MESH_URI_HASH_LEN                                     4
#define MIBLE_MESH_SEND_LINK_OPEN_TIMEOUT                           10000
#define MIBLE_MESH_PROVISONE_TIME_OUT                               60000
#define MIBLE_MESH_GET_OPTION                                       0
#define MIBLE_MESH_SET_OPTION                                       1

/*** mesh standard provision define***/
#define MIBLE_MESH_CONFIRMAITON_KEY_LEN                             16
#define MIBLE_MESH_PROVISION_RANDOM_LEN                             16
#define MIBLE_MESH_PROVISION_CONFIRMATION_LEN                       16

#define MIBLE_MESH_KEY_LEN                                          16
#define MIBLE_MESH_DEVICEKEY_LEN                                    16
#define MIBLE_MESH_LTMK_SIGNATURE_LEN                               64
#define MIBLE_MESH_LTMK_LEN                                         16
#define MIBLE_MESH_OOB_DATA_LEN                                     20
#define MIBLE_MESH_PUB_LEN                                          64

/* mible_mesh_event and callback data */
typedef enum {
    MIBLE_MESH_EVENT_STACK_INIT_DONE=0,     /**< NULL */
    MIBLE_MESH_EVENT_PROVISIONER_INIT_DONE, /**< NULL */
    MIBLE_MESH_EVENT_ADV_PACKAGE,           /**< Deprecated event */
    MIBLE_MESH_EVENT_UNPROV_DEVICE,         /**< mible_mesh_unprov_beacon_t */
    MIBLE_MESH_EVENT_IV_UPDATE,             /**< mible_mesh_iv_t */
    MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB,     /**< Mesh Profile definition message */
    MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB,    /**< Mesh Model definition message */
    MIBLE_MESH_EVENT_CONFIRMATION_KEY,      /**< mible_mesh_confirmation_key_t*/
    MIBLE_MESH_EVENT_DEV_RAND_CONFIRM,      /**< mible_mesh_device_random_confirmation_t*/
    MIBLE_MESH_EVENT_PROV_RESULT,           /**< mible_mesh_provision_result_t*/
} mible_mesh_event_type_t;

/**
 * @brief mible mesh provision type.
 */
typedef enum {
    MIBLE_MESH_PB_ADV,
    MIBLE_MESH_PB_GATT,
} mible_mesh_pb_type_t;

/**
 * @brief mible mesh address type.
 */
typedef enum {
    MIBLE_MESH_ADDRESS_TYPE_UNASSIGNED = 0, /**< 0b0000000000000000 */
    MIBLE_MESH_ADDRESS_TYPE_UNICAST,        /**< 0b0xxxxxxxxxxxxxxx (excluding 0b0000000000000000) */
    MIBLE_MESH_ADDRESS_TYPE_VIRTUAL,        /**< 0b10xxxxxxxxxxxxxx */
    MIBLE_MESH_ADDRESS_TYPE_GROUP,          /**< 0b11xxxxxxxxxxxxxx */
} mible_mesh_address_type_t;

/**
 * @brief provisioner random and confirmation.
 */
typedef struct{
    uint8_t uuid[MIBLE_MESH_DEV_UUID_LEN];                                /** provision uuid */
    uint8_t provision_random[MIBLE_MESH_PROVISION_RANDOM_LEN];            /** provisioner random */
    uint8_t provision_confimation[MIBLE_MESH_PROVISION_CONFIRMATION_LEN]; /** provisioner confirmation */
}mible_mesh_prov_rand_confirm_t;

/**
 * @brief provision authentication result.
 */
typedef struct{
    uint8_t result;                          /** provision result */
    uint8_t flags;                           /** network flags */
    uint16_t uincast_address;                /** unicast address */
    uint16_t netkey_index;                   /** netkey index */
    uint32_t iv_index;                       /** iv index */
    uint8_t netkey[MIBLE_MESH_KEY_LEN];           /** net key */
    uint8_t uuid[MIBLE_MESH_DEV_UUID_LEN];   /** provision uuid */
}mible_mesh_authentication_result_t;

/**
 * @brief mible mesh address description.
 */
typedef struct {
    uint16_t type;                  /**< mible_mesh_address_type_t */
    uint16_t value;                 /**< mesh address */
    const uint8_t *virtual_uuid;    /**< virtual uuid value, must be NULL unless address type is #MIBLE_MESH_ADDRESS_TYPE_VIRTUAL */
} mible_mesh_address_t;

/**
 * @brief mible mesh model description.
 */
typedef struct {
    uint16_t model_id;
    uint16_t company_id;
} __packed mible_mesh_model_id_t;

/**
 * @brief mible opcode description
 * you can't omit bit flag 0b0, 0b10, 0b11 for opcode.
 * Opcode   Format Notes
 * 0xxxxxxx (excluding 01111111) 1-octet Opcodes
 * 10xxxxxx xxxxxxxx 2-octet Opcodes
 * 11xxxxxx zzzzzzzz 3-octet Opcodes
 */
typedef struct {
    uint16_t company_id;    /**< SIG ID: 0xFFFF */
    uint16_t opcode;        /**< operation code, defined mesh profile 3.7.3.1 Operation codes */
} __packed mible_mesh_opcode_t;

/**
 * @brief mible opcode database for vendor model id.
 */
typedef struct {
    mible_mesh_model_id_t model_id;
    uint16_t opcode_num;
    uint16_t *opcode;
} mible_mesh_model_opcodes_t;

/**
 * @brief mible vendor model id database.
 */
typedef struct {
    uint16_t model_num;     /**< vendor model number */
    mible_mesh_model_opcodes_t *model_list; /**< vendor model opcode list */
} mible_mesh_model_db_t;

/**
 * @brief mible sig model id database.
 */
typedef struct {
    uint16_t model_num;     /**< sig model number */
    uint16_t *model_list;   /**< sig model list */
} mible_mesh_sig_model_db_t;

/**
 * @brief mible mesh init data.
 * flags    IV Update Flag     0: Normal operation 1: IV Update active
 */
typedef struct{
    uint16_t unicast_address;       /**< gateway unicast address */
    uint16_t group_address;         /**< subscription group address for all initial models */
    uint32_t iv_index;              /**< iv index vaule */
    uint8_t  flags;                 /**< iv update flags*/
    uint16_t netkey_index;          /**< gloabl netkey index */
    //uint16_t appkey_index;          /**< global netkey index */
    uint8_t  primary_netkey[MIBLE_MESH_KEY_LEN];    /**< netkey */
    //uint8_t  primary_appkey[MIBLE_MESH_KEY_LEN];    /**< appkey */
    uint8_t  max_num_netkey;        /**< stack support netkey num */
    uint8_t  max_num_appkey;        /**< stack support appkey num */
    uint16_t replay_list_size;      /**< replay protection list size */
    uint16_t default_ttl;           /**< default ttl */
    mible_mesh_sig_model_db_t sig_model_db;    /**< sig models database */
    mible_mesh_model_db_t vendor_model_db; /**< vendor models database */
}mible_mesh_gateway_info_t;

/**
 * @brief mible mesh iv info.
 */
typedef struct {
    uint32_t iv_index;  /**< mesh network iv index */
    uint8_t  flags;     /**< IV Update Flag     0: Normal operation 1: IV Update active*/
} mible_mesh_iv_t;

/**
 * @brief netkey add/update/delete params.
 */
typedef struct{
    uint16_t dst_addr;
    uint16_t netkey_index;
    uint8_t  netkey[MIBLE_MESH_KEY_LEN];
    uint16_t global_netkey_index;       /**< local encrypt netkey index for mesh message */
}mible_mesh_netkey_params_t;

/**
 * @brief   appkey add/update/delete params.
 *          global_appkey_index: if your stack can use appkey[] send directly msg, this parameter is redundant;
 *          some stacks need to find appkey by global_appkey_index, and then send out add ... nesh msg.
 */
typedef struct{
    uint16_t dst_addr;
    uint16_t netkey_index;
    uint16_t appkey_index;
    uint8_t  appkey[MIBLE_MESH_KEY_LEN];
    uint16_t global_netkey_index;       /**< local encrypt netkey index for mesh message */
    uint16_t global_appkey_index;       /**< mesh spec don't need this param, only passed for special mesh stack*/
}mible_mesh_appkey_params_t;

/**
 * @brief model app bind/unbind params.
 */
typedef struct{
    uint16_t dst_addr;
    uint16_t element_addr;
    mible_mesh_model_id_t model_id;      /**< mible_mesh_model_id_t */
    uint16_t appkey_index;
    uint16_t global_netkey_index;       /**< local encrypt netkey index for mesh message */
}mible_mesh_model_app_params_t;

/**
 * @brief subscription add/delete/overwrite/delete_all params.
 */
typedef struct{
    uint16_t dst_addr;
    uint16_t element_addr;
    mible_mesh_model_id_t model_id;
    mible_mesh_address_t sub_addr;
    uint16_t global_netkey_index;       /**< local encrypt netkey index for mesh message */
}mible_mesh_subscription_params_t;

/**
 * @brief publication add/delete/overwrite/delete_all params.
 */
typedef struct{
    uint16_t dst_addr;
    uint16_t element_addr;
    mible_mesh_model_id_t model_id;
    uint16_t appkey_index;
    mible_mesh_address_t publish_addr;
    uint8_t  pub_ttl;
    uint8_t  credential_flag;
    uint8_t  pub_period;
    uint8_t  pub_retrans_count;
    uint8_t  pub_retrans_intvl_steps;
    uint16_t global_netkey_index;       /**< local encrypt netkey index for mesh message */
}mible_mesh_publication_params_t;

/**
 * @brief reset node params.
 */
typedef struct{
    uint16_t dst_addr;
    uint16_t global_netkey_index;       /**< local encrypt netkey index for mesh message */
}mible_mesh_reset_params_t;

/**
 * @brief relay get/set params.
 */
typedef struct{
    uint16_t dst_addr;
    uint8_t  relay;
    uint8_t  retransmit_count;
    uint8_t  retransmit_interval_steps;
    uint16_t global_netkey_index;       /**< local encrypt netkey index for mesh message */
}mible_mesh_relay_params_t;

/**
 * @brief generic message params, like on_off/lightness/...
 * adv_timeout_ms == 0 or adv_interval_ms == 0, use default adv interval and timeout for mesh message
 */
typedef struct{
    mible_mesh_address_t dst_addr;
    uint16_t global_netkey_index;
    uint16_t global_appkey_index;
    mible_mesh_opcode_t opcode;
    uint32_t adv_timeout_ms;    /**< mesh message advertising timeout, unit: ms */
    uint8_t  adv_interval_ms;   /**< mesh message adv interval, unit: ms */
    uint8_t  data_len;
    uint8_t* data;      /**< mesh message raw data */
}mible_mesh_generic_params_t;

/**
 * @brief provision device info.
 */
typedef struct {
    uint8_t address[MIBLE_ADDR_LEN];
    uint8_t address_type;
    uint8_t device_type;
    uint8_t pbtranstype;    /* provision type */
}mible_mesh_remote_device_info_t;

/**
 * @brief unprovision beacon.
 */
typedef struct{
    uint8_t  device_uuid[MIBLE_MESH_DEV_UUID_LEN];
    uint16_t oob_info;
    uint8_t  uri_hash[MIBLE_MESH_URI_HASH_LEN];
    uint8_t  mac[MIBLE_ADDR_LEN];
}mible_mesh_unprov_beacon_t;

/**
 * @brief mesh message meta data.
 */
typedef struct {
    uint16_t src_addr;      /**< [mandatary]  source address */
    uint16_t dst_addr;      /**< [mandatary]  maybe group address, or provisioner addr */
    uint16_t appkey_index;  /**< [mandatary]  appkey index for this message */
    uint16_t netkey_index;  /**< [optional]  if not, default value 0xFFFF */
    int8_t  rssi;           /**< [optional]  if not, default value -1 */
    uint8_t  ttl;           /**< [optional]  if not, default value 0 */
} mible_mesh_message_rx_meta_t;

/**
 * @brief mesh message.
 */
typedef struct {
    mible_mesh_opcode_t opcode;
    mible_mesh_message_rx_meta_t meta_data;
    uint16_t buf_len;       /* mesh raw data len*/
    uint8_t* buf;           /* mesh raw data, see mesh profile 4.3 */
} mible_mesh_access_message_rx_t;

/**
 * @brief provisioner confimation key.
 */
typedef struct{
    uint8_t uuid[MIBLE_MESH_DEV_UUID_LEN];       /** mesh std provision uuid */
    uint8_t confirmation_key[MIBLE_MESH_CONFIRMAITON_KEY_LEN];   /** mesh std provision confirmation key */
}mible_mesh_confirmation_key_t;

/**
 * @brief device random and confirmation.
 */
typedef struct{
    uint8_t uuid[MIBLE_MESH_DEV_UUID_LEN];           /** mesh std provision uuid */
    uint8_t device_random[MIBLE_MESH_PROVISION_RANDOM_LEN]; /** device random */
    uint8_t device_confirmation[MIBLE_MESH_PROVISION_CONFIRMATION_LEN];/** device confirmation */
}mible_mesh_device_random_confirmation_t;

/**
 * @brief provision result.
 */
typedef struct{
    uint8_t result;                               /** mesh provision result */
    uint8_t uuid[MIBLE_MESH_DEV_UUID_LEN];        /** mesh std provision uuid */
    uint8_t device_key[MIBLE_MESH_DEVICEKEY_LEN]; /** device key */
}mible_mesh_provision_result_t;

/**
 * @brief ADD contains add and update operation.
 *        This is only used to mesh provisioner local opertions for mible_mesh_gateway_*.
 *        DO NOT BE USE TO mible_mesh_node_* functions.
 *        Firstly, you should execute get-operation,
 *        and then, according to get result, choose you really add or update method.
 *        if get-operation return null, you can call add method, otherwise, call update method.
 */
typedef enum {
    MIBLE_MESH_OP_ADD=0,
    MIBLE_MESH_OP_DELETE,
}mible_mesh_op_t;

/**
 * @brief mible mesh uuid definition.
 */
typedef struct {
    uint16_t cid;       /**< company id */
    uint16_t pdid;      /**< product id */
    uint16_t reserved;  /**< reserved for future */
    uint8_t  did[4];    /**< device id lower 4 bytes */
    uint8_t  mac[MIBLE_ADDR_LEN];   /**< mac address */
} __packed mible_mesh_uuid_t;

/**
 * @brief mesh device information.
 */
typedef struct {
    uint16_t unicast_address;
    uint16_t elements_num;
    uint8_t device_key[MIBLE_MESH_KEY_LEN];
    uint8_t uuid[MIBLE_MESH_DEV_UUID_LEN];
    uint16_t netkey_index;
}mible_mesh_node_info_t;


/**********************************************************************//**
 * Config Message Status Definitions
 **********************************************************************/

/**
 * @brief Mesh Config Beacon Status event paramater type.
 */
typedef struct
{
    uint8_t beacon;
} mible_mesh_conf_beacon_status_t;

/**
 * @brief Mesh Config Composition Data Status event paramater type.
 */
typedef struct
{
    uint8_t page;
    uint16_t company_id;
    uint16_t product_id;
    uint16_t version_id;
    uint16_t crpl;
    uint16_t features;
    uint16_t data_len;
    void *data;
} mible_mesh_conf_compo_data_status_t;

/**
 * @brief Mesh Config Default TTL Status event paramater type.
 */
typedef struct
{
    uint8_t ttl;
} mible_mesh_conf_def_ttl_status_t;

/**
 * @brief Mesh Config GATT Proxy Status event paramater type.
 */
typedef struct
{
  uint8_t gatt_proxy;
} mible_mesh_conf_gatt_proxy_status_t;

/**
 * @brief Mesh Config Relay Status event paramater type.
 */
typedef struct
{
    uint8_t relay;
    uint8_t relay_retrans_cnt;
    uint8_t relay_retrans_intvlsteps;
} mible_mesh_conf_relay_status_t;

/**
 * @brief Mesh Config Model Publication Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t elem_addr;
    uint16_t pub_addr;
    uint16_t appkey_index;
    uint8_t cred_flag;
    uint8_t pub_ttl;
    uint8_t pub_perid;
    uint8_t pub_retrans_cnt;
    uint8_t pub_retrans_intvl_steps;
    mible_mesh_model_id_t model_id;
} mible_mesh_conf_model_pub_status_t;

/**
 * @brief Mesh Config Model Sublication Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t elem_addr;
    uint16_t address;
    mible_mesh_model_id_t model_id;
} mible_mesh_conf_model_sub_status_t;

/**
 * @brief Mesh Config Sig Model Sublication List event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t elem_addr;
    uint16_t sig_model_id;
    uint16_t num;
    uint16_t *addresses;
} mible_mesh_conf_sig_model_sub_list_t;

/**
 * @brief Mesh Config Vendor Model Sublication List event paramater type.
 */
typedef struct
{
  uint8_t status;
  uint16_t elem_addr;
  mible_mesh_model_id_t vnd_model_id;
  uint16_t num;
  uint16_t *addresses;
} mible_mesh_conf_vnd_model_sub_list_t;

/**
 * @brief Mesh Config NetKey Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t netkey_index;
} mible_mesh_conf_netkey_status_t;

/**
 * @brief Mesh Config NetKey List event paramater type.
 */
typedef struct
{
    uint16_t num_of_netkey;
    uint16_t *pnetkeyindexes;
} mible_mesh_conf_netkey_list_t;

/**
 * @brief Mesh Config AppKey List event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t netkey_index;
    uint16_t num_of_appkey;
    uint16_t *pappkeyindexes;
} mible_mesh_conf_appkey_list_t;

/**
 * @brief Mesh Config AppKey Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t netkey_index;
    uint16_t appkey_index;
} mible_mesh_conf_appkey_status_t;

/**
 * @brief Mesh Config Node Identity Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t netkey_index;
    uint8_t identity;
} mible_mesh_conf_node_ident_status_t;

/**
 * @brief Mesh Config Model App Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t elem_addr;
    uint16_t appkey_index;
    mible_mesh_model_id_t model_id;
} mible_mesh_conf_model_app_status_t;

/**
 * @brief Mesh Config SIG Model App List event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t elem_addr;
    uint16_t model_id;
    uint16_t num_of_appkey;
    uint16_t *pappkeyindexes;
} mible_mesh_conf_sig_model_app_list_t;

/**
 * @brief Mesh Config Vendor Model App List event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t elem_addr;
    mible_mesh_model_id_t vnd_model_id;
    uint16_t num_of_appkey;
    uint16_t *pappkeyindexes;
} mible_mesh_conf_vnd_model_app_list_t;

/**
 * @brief Mesh Config Friend Status event paramater type.
 */
typedef struct
{
    uint8_t friend;
} mible_mesh_conf_frnd_status_t;

/**
 * @brief Mesh Config Key Refresh Phase  Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t netkey_index;
    uint8_t phase;
} mible_mesh_conf_keyrefresh_phase_status_t;

/**
 * @brief Mesh Config    Heartbeat Publication Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t dest_addr;
    uint8_t count_log;
    uint8_t period_log;
    uint8_t ttl;
    uint16_t features;
    uint16_t netkey_index;
} mible_mesh_conf_hb_pub_status_t;

/**
 * @brief Mesh Config    Heartbeat Subscription Status event paramater type.
 */
typedef struct
{
    uint8_t status;
    uint16_t src_addr;
    uint16_t dst_addr;
    uint8_t period_log;
    uint8_t count_log;
    uint8_t min_hops;
    uint8_t max_hops;
} mible_mesh_conf_hb_sub_status_t;

/**
 * @brief Mesh Config Low Power Node PollTimeout Status event paramater type.
 */
typedef struct
{
    uint16_t lpn_addr;
    uint32_t polltimeout;
} mible_mesh_conf_lpn_polltimeout_status_t;

/**
 * @brief Mesh Config Network Transmit Status event paramater type.
 */
typedef struct
{
    uint8_t nwk_transcnt;
    uint8_t nwk_trans_intvl_steps;
} mible_mesh_conf_nwk_trans_status_t;

/**
 * @brief Mesh Health Current/Fault Status event paramater type.
 */
typedef struct
{
  uint8_t test_id;
  uint16_t company_id;
  uint8_t fault_num;
  uint8_t *fault_array;
} mible_mesh_hlth_fault_status_t;

/**
 * @brief Mesh Health Period Status event paramater type.
 */
typedef struct
{
  uint8_t fast_peirod_div;
} mible_mesh_hlth_period_status_t;

/**
 * @brief Mesh Health Attention Status event paramater type.
 */
typedef struct
{
  uint8_t attention;
} mible_mesh_hlth_atten_status_t;

/**
 * @brief Mesh Model Client event paramater type.
 * you should implement all status corresponsing to mible_mesh_node_*.
 */
typedef struct
{
    mible_mesh_opcode_t opcode;
    mible_mesh_message_rx_meta_t meta_data;
    union{
        mible_mesh_conf_beacon_status_t beacon_status;
        mible_mesh_conf_compo_data_status_t compo_data_status;
        mible_mesh_conf_def_ttl_status_t def_ttl_status;
        mible_mesh_conf_gatt_proxy_status_t gatt_proxy_status;
        mible_mesh_conf_relay_status_t relay_status;
        mible_mesh_conf_model_pub_status_t model_pub_status;
        mible_mesh_conf_model_sub_status_t model_sub_status;
        mible_mesh_conf_sig_model_sub_list_t sig_model_sub_list;
        mible_mesh_conf_vnd_model_sub_list_t vnd_model_sub_list;
        mible_mesh_conf_netkey_status_t netkey_status;
        mible_mesh_conf_netkey_list_t netkey_list;
        mible_mesh_conf_appkey_list_t appkey_list;
        mible_mesh_conf_appkey_status_t appkey_status;
        mible_mesh_conf_node_ident_status_t node_ident_status;
        mible_mesh_conf_model_app_status_t model_app_status;
        mible_mesh_conf_sig_model_app_list_t sig_model_app_list;
        mible_mesh_conf_vnd_model_app_list_t vnd_model_app_list;
        mible_mesh_conf_frnd_status_t frnd_status;
        mible_mesh_conf_keyrefresh_phase_status_t keyrefresh_phase_status;
        mible_mesh_conf_hb_pub_status_t hb_pub_status;
        mible_mesh_conf_hb_sub_status_t hb_sub_status;
        mible_mesh_conf_lpn_polltimeout_status_t lpn_polltimeout_status;
        mible_mesh_conf_nwk_trans_status_t nwk_trans_status;

        mible_mesh_hlth_fault_status_t hlth_fault_status;
        mible_mesh_hlth_period_status_t hlth_period_status;
        mible_mesh_hlth_atten_status_t hlth_atten_status;
    };
}mible_mesh_config_status_t;


typedef union {
    mible_gap_adv_report_t adv_report;  /**< Deprecated event */
    mible_mesh_unprov_beacon_t unprov_beacon;
    mible_mesh_iv_t mesh_iv;
    mible_mesh_config_status_t config_msg;
    mible_mesh_access_message_rx_t generic_msg;
    mible_mesh_confirmation_key_t confirm_key;
    mible_mesh_device_random_confirmation_t dev_rand_confirm;
    mible_mesh_provision_result_t prov_result;
} mible_mesh_event_params_t;


/**
 *@brief    mible mesh event callback, report to applications
 *@param    [in] type : mible_mesh_event_type_t definitions
 *@param    [in] data : mible_mesh_event_params_t pointer.
 *@return   0: success, negetive value: failure
 */
typedef int (*mible_mesh_event_cb_t)(mible_mesh_event_type_t type, mible_mesh_event_params_t *data);

/**********************************************************************//**
 * Provisioner interacts with node by air interface.
 * opcode: mesh spec define opcode
 * global_netkey_index: local key index, is used to encrypt network data;
 * global_appkey_index: local key index, is used to encrypt app data;
 * netkey_index: mesh spec define payload in mesh message raw data
 * appkey_index: mesh spec define payload in mesh message raw data
 **********************************************************************/

/**
 *@brief    Config Composition Data Get, mesh profile 4.3.2.4, Report 4.3.2.5 Config Composition Data Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@param    [in] unicast_address: node address
 *@param    [in] netkey_index: key index for node
 *@param    [in] page: page number of the composition data
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_get_composition_data(uint16_t unicast_address, uint16_t global_netkey_index, uint8_t page);

/**
 *@brief    appkey information for node, mesh profile 4.3.2.37-39, Report 4.3.2.40 Config AppKey Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@param    [in] opcode : mesh spec opcode, add/update/delete ...
 *@param    [in] param : appkey parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_appkey(uint16_t opcode, mible_mesh_appkey_params_t *param);

/**
 *@brief    bind appkey information for node, mesh profile 4.3.2.46-47, Report 4.3.2.48 Config Model App Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@param    [in] opcode : mesh spec opcode, bind/unbind
 *@param    [in] param : bind parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_bind_appkey(uint16_t opcode, mible_mesh_model_app_params_t *param);

/**
 *@brief    set publication information for node, mesh profile 4.3.2.15-17,
 *          Report 4.3.2.18 Config Model Publication Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@param    [in] opcode : mesh spec opcode, add/delete/overwrite ...
 *@param    [in] param : publish parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_publication(uint16_t opcode, mible_mesh_publication_params_t * param);

/**
 *@brief    set subscription information for node, mesh profile 4.3.2.19-25.
 *          Report 4.3.2.26 Config Model Subscription Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@param    [in] opcode : mesh spec opcode, add/delete/overwrite ...
 *@param    [in] param : subscription parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_subscription(uint16_t opcode, mible_mesh_subscription_params_t * param);

/**
 *@brief    reset node, 4.3.2.53 Config Node Reset, Report 4.3.2.54 Config Node Reset Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@param    [in] opcode : mesh spec opcode, reset
 *@param    [in] param : reset parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_reset(uint16_t opcode, mible_mesh_reset_params_t *param);

/**
 *@brief    set relay params node, mesh profile 4.3.2.12, Report 4.3.2.14 Config Relay Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@param    [in] opcode : relay
 *@param    [in] param : relay parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_relay_param(uint16_t opcode, mible_mesh_relay_params_t *param);

/**
 *@brief    generic message, Mesh model 3.2, 4.2, 5.2, 6.3, or 7 Summary.
 *          report event: MIBLE_MESH_EVENT_GENERIC_OPTION_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] param : control parameters corresponding to node
 *          according to opcode, generate a mesh message; extral params: ack_opcode, tid, get_or_set.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_generic_control(mible_mesh_generic_params_t * param);


/**********************************************************************//**
 * Provisioner Local Operation Definitions
 * netkey_index: local key index, global_netkey_index, is used to encrypt network data;
 * appkey_index: local key index, global_appkey_index, is used to encrypt app data;
 * #####################################################################
 * Prerequisites: our provisioner has only one element address called unicast_address,
 * all message will be send out by [element_index = 0]'s unicast_address.
 * #####################################################################
 **********************************************************************/

/**
 *@brief    sync method, register event callback
 *@param    [in] mible_mesh_event_cb : event callback
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_register_event_callback(mible_mesh_event_cb_t mible_mesh_event_cb);

/**
 *@brief    sync method, unregister event callback
 *@param    [in] mible_mesh_event_cb : event callback
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_unregister_event_callback(mible_mesh_event_cb_t mible_mesh_event_cb);

/**
 *@brief    async method, init mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_INIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_init_stack(void);

/**
 *@brief    deinit mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_DEINIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_deinit_stack(void);

/**
 *@brief    async method, init mesh provisioner
 *          load self info, include unicast address, iv, seq_num, init model;
 *          clear local db, related appkey_list, netkey_list, device_key_list,
 *          we will load latest data for cloud;
 *          report event: MIBLE_MESH_EVENT_PROVISIONER_INIT_DONE, data: NULL.
 *@param    [in] info : init parameters corresponding to gateway
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_init_provisioner(mible_mesh_gateway_info_t *info);

/**
 *@brief    sync method, acquire reload_flag
 *          if you are a transciver device, always return flag = 1,
 *          we will load latest data from application;
 *          if you automatically reload data from flash or others,
 *          we will skip create_network, set_appkey, set_model_app, default group sub;
 *@return    0: do not need to reload, 1: need to reload, negetive value: failure
 */
int mible_mesh_gateway_get_reload_flag(void);

/**
 *@brief    sync method, create mesh network for provisioner.
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] netkey : netkey value
 *@param    [in|out] stack_netkey_index : [in] default value: 0xFFFF, [out] stack generates netkey_index
 *          if your stack don't manage netkey_index and stack_netkey_index relationships, update stack_netkey_index;
 *          otherwise, do nothing.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_create_network(uint16_t netkey_index, uint8_t *netkey, uint16_t *stack_netkey_index);

/**
 *@brief    set local provisioner network transmit params.
 *@param    [in] count : advertise counter for every adv packet, adv transmit times
 *@param    [in] interval_steps : adv interval = interval_steps*0.625ms
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_network_transmit_param(uint8_t count, uint8_t interval_steps);

/**
 *@brief    start recv unprovision beacon, report result by MIBLE_EVENT.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_start_recv_unprovbeacon(void);

/**
 *@brief    stop recv unprovision beacon, terminate report result.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_stop_recv_unprovbeacon(void);

/**
 *@brief    update iv index, .
 *@param    [in] iv_index : current IV Index
 *@param    [in] flags : IV Update Flag     0: Normal operation 1: IV Update active
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_update_iv_info(uint32_t iv_index, uint8_t flags);

/**
 *@brief    add/delete local netkey.
 *@param    [in] op : add or delete
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] netkey : netkey value
 *@param    [in|out] stack_netkey_index : [in] default value: 0xFFFF, [out] stack generates netkey_index
 *          if your stack don't manage netkey_index and stack_netkey_index relationships, update stack_netkey_index.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_netkey(mible_mesh_op_t op, uint16_t netkey_index, uint8_t *netkey,
            uint16_t *stack_netkey_index);

/**
 *@brief    add/delete local appkey.
 *@param    [in] op : add or delete
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] appkey_index : key index for appkey
 *@param    [in] appkey : appkey value
 *@param    [in|out] stack_appkey_index : [in] default value: 0xFFFF, [out] stack generates appkey_index
 *          if your stack don't manage appkey_index and stack_appkey_index relationships, update stack_appkey_index.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_appkey(mible_mesh_op_t op, uint16_t netkey_index, uint16_t appkey_index,
            uint8_t * appkey, uint16_t *stack_appkey_index);

/**
 *@brief    bind/unbind model app.
 *@param    [in] op : bind is MIBLE_MESH_OP_ADD, unbind is MIBLE_MESH_OP_DELETE
 *@param    [in] company_id: company id
 *@param    [in] model_id : model_id
 *@param    [in] appkey_index : key index for appkey
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_model_app(mible_mesh_op_t op, uint16_t company_id, uint16_t model_id,
            uint16_t appkey_index);

/**
 *@brief    add/delete device key.
 *@param    [in] op : add or delete
 *@param    [in] unicast_address: remote device unicast address
 *@param    [in] device_key : device key value
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_device_key(mible_mesh_op_t op, mible_mesh_node_info_t *device);

/**
 *@brief    add/delete subscription params.
 *@param    [in] op : add or delete
 *@param    [in] company_id: company id
 *@param    [in] model_id : model_id
 *@param    [in] sub_addr: subscription address params
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_sub_address(mible_mesh_op_t op, uint16_t company_id, uint16_t model_id,
        mible_mesh_address_t *sub_addr);

/**
 *@brief    start provision.
 *@param    [in] uuid : the device to start provision
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_start_provision(uint8_t *uuid);

/**
 *@brief    set provisioner random and confirmation.
 *@param    [in] param : the prov random confirm structure 
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_provisioner_rand_confirm(mible_mesh_prov_rand_confirm_t *param);

/**
 *@brief    set provision authentication result.
 *@param    [in] param : the check rsult structure 
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_authentication_result(mible_mesh_authentication_result_t *param);

/**
 *@brief    suspend adv send for mesh stack.
 *@param    [in] NULL
 *@return   0: success, negetive value: failure
 */
int mible_mesh_suspend_transmission(void);

/**
 *@brief    resume adv send for mesh stack.
 *@param    [in] NULL
 *@return   0: success, negetive value: failure
 */
int mible_mesh_resume_transmission(void);

#ifdef __cplusplus
}
#endif

#endif

