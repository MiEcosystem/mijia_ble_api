#ifndef __MIBLE_MESH_API_H
#define __MIBLE_MESH_API_H

#ifdef __cplusplus
extern "C" {
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
#define MIBLE_MESH_MODEL_ID_LIGHTNESS_CLIENT                         0x1302
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
#define MIBLE_MESH_MSG_GENERIC_ONOFF_GET               				0x8201
#define MIBLE_MESH_MSG_GENERIC_ONOFF_SET               				0x8202
#define MIBLE_MESH_MSG_GENERIC_ONOFF_SET_UNACKNOWLEDGED    			0x8203
#define MIBLE_MESH_MSG_GENERIC_ONOFF_STATUS            				0x8204

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


/** Mesh vendor models **/
#define MIBLE_MESH_MIOT_SPEC_SERVER_MODEL                           0x038f0000
#define MIBLE_MESH_MIOT_SPEC_CLIENT_MODEL                           0x038f0001
#define MIBLE_MESH_MIJIA_SERVER_MODEL                               0x038f0002
#define MIBLE_MESH_MIJIA_CLIENT_MODEL                               0x038f0003

/*** Mesh vendor opcodes ***/
#define MIBLE_MESH_MIOT_SPEC_GET                                    0xC1038F
#define MIBLE_MESH_MIOT_SPEC_SET                                    0xC3038F
#define MIBLE_MESH_MIOT_SPEC_SET_NOACK                              0xC4038F
#define MIBLE_MESH_MIOT_SPEC_STATUS                                 0xC5038F

#define MIBLE_MESH_COMPANY_ID                                       0x038F
#define MIBLE_MESH_ADV_PKT_INTERNAL                                 40 //48
#define MIBLE_MESH_ADV_PKT_COUNTER                                  3

#define MIBLE_MESH_DEV_UUID_LEN                                     16
#define MIBLE_MESH_URI_HASH_LEN                                     4
#define MIBLE_MESH_SEND_LINK_OPEN_TIMEOUT                           10000
#define MIBLE_MESH_PROVISONE_TIME_OUT                               60000
#define MIBLE_MESH_GET_OPTION                                       0
#define MIBLE_MESH_SET_OPTION                                       1

#define MIBLE_MESH_KEY_LEN                                          16
#define MIBLE_MESH_DEVICEKEY_LEN                                    16
#define MIBLE_MESH_LTMK_SIGNATURE_LEN                               64
#define MIBLE_MESH_LTMK_LEN                                         16
#define MIBLE_MESH_OOB_DATA_LEN                                     20
#define MIBLE_MESH_PUB_LEN                                          64

/* mible_mesh_event and callback data */
typedef enum {
    MIBLE_MESH_EVENT_STACK_INIT_DONE=0,   /* NULL */
    MIBLE_MESH_EVENT_LOAD_CONFIG_DONE,
    MIBLE_MESH_EVENT_ADV_PACKAGE,       /* mible_gap_adv_report_t */
    MIBLE_MESH_EVENT_UNPROV_DEVICE,     /* mible_mesh_unprov_beacon_t*/
    MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, /* Mesh Profile definition message */
    MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, /* Mesh Model definition message */
} mible_mesh_event_type_t;

/* provision type */
typedef enum {
    MIBLE_MESH_PB_ADV,
    MIBLE_MESH_PB_GATT,
} mible_mesh_pb_type_t;

/* mible mesh address type */
typedef enum {
    MIBLE_MESH_ADDRESS_TYPE_UNASSIGNED = 0,
    MIBLE_MESH_ADDRESS_TYPE_UNICAST,
    MIBLE_MESH_ADDRESS_TYPE_VIRTUAL,
    MIBLE_MESH_ADDRESS_TYPE_GROUP,
} mible_mesh_address_type_t;

/* mible mesh address description */
typedef struct {
    mible_mesh_address_type_t type;
    uint16_t value;
    const uint8_t *virtual_uuid;    /**< virtual uuid value, must be NULL unless address type is #MIBLE_MESH_ADDRESS_TYPE_VIRTUAL */
} mible_mesh_address_t;

/* mible mesh model description, uint32_t model_id*/
typedef struct {
    uint16_t model_id;
    uint16_t company_id;
} mible_mesh_model_id_t;

/* mible opcode description, uint32_t opcode*/
typedef struct {
    uint16_t company_id;
    uint16_t opcode;
} mible_mesh_opcode_t;

/* mible opcode database for special model id */
typedef struct {
    uint32_t model_id;
    uint32_t opcode_num;
    uint32_t *opcode;
} mible_mesh_model_opcodes_t;

/* mible vendor model id database */
typedef struct {
    uint32_t model_num;
    mible_mesh_model_opcodes_t *model_list;
} mible_mesh_model_db_t;

typedef struct {
    uint32_t model_num;
    uint32_t *model_list;
} mible_mesh_sig_model_db_t;

/* mible gateway init data*/
typedef struct{
    uint16_t unicast_address;       /* gateway unicast address */
    uint16_t group_address;         /* subscription group address for all initial models */
    uint32_t iv_index;              /* iv index vaule*/
    int32_t  iv_flags;
    uint16_t netkey_index;
    uint16_t appkey_index;
    uint8_t  primary_netkey[MIBLE_MESH_KEY_LEN];
    uint8_t  primary_appkey[MIBLE_MESH_KEY_LEN];
    uint8_t  max_num_netkey;        /* stack support netkey num */
    uint8_t  max_num_appkey;        /* stack support appkey num */
    uint16_t replay_list_size;      /* replay protection list size */
    uint16_t default_ttl;           /* default ttl */
    mible_mesh_sig_model_db_t sig_model_db;    /* sig models database */
    mible_mesh_model_db_t vendor_model_db; /* vendor models database */
}mible_mesh_gateway_info_t;

/* netkey add/update/delete params */
typedef struct{
    uint16_t dst_addr;
    uint16_t netkey_index;
    uint8_t  netkey[MIBLE_MESH_KEY_LEN];
    uint16_t global_netkey_index;       /* local encrypt netkey index for mesh message */
}mible_mesh_netkey_params_t;

/* appkey add/update/delete params */
typedef struct{
    uint16_t dst_addr;
    uint16_t netkey_index;
    uint16_t appkey_index;
    uint8_t  appkey[MIBLE_MESH_KEY_LEN];
    uint16_t global_netkey_index;       /* local encrypt netkey index for mesh message */
    uint16_t global_appkey_index;       /* local appkey index */
}mible_mesh_appkey_params_t;

/* model app bind/unbind params */
typedef struct{
    uint16_t dst_addr;
    uint16_t element_addr;
    uint32_t model_id;      /* mible_mesh_model_id_t */
    uint16_t appkey_index;
    uint16_t global_netkey_index;       /* local encrypt netkey index for mesh message */
}mible_mesh_model_app_params_t;

/* subscription add/delete/overwrite/delete_all params */
typedef struct{
    uint16_t dst_addr;
    uint16_t element_addr;
    uint32_t model_id;
    mible_mesh_address_t sub_addr;
    uint16_t global_netkey_index;       /* local encrypt netkey index for mesh message */
}mible_mesh_subscription_params_t;

/* publication add/delete/overwrite/delete_all params */
typedef struct{
    uint16_t dst_addr;
    uint16_t element_addr;
    uint32_t model_id;
    uint16_t appkey_index;
    mible_mesh_address_t publish_addr;
    uint8_t  pub_ttl;
    uint8_t  credential_flag;
    uint8_t  pub_period;
    uint8_t  pub_retrans_count;
    uint8_t  pub_retrans_intvl_steps;
    uint16_t global_netkey_index;       /* local encrypt netkey index for mesh message */
}mible_mesh_publication_params_t;

/* reset node params */
typedef struct{
    uint16_t dst_addr;
    uint16_t global_netkey_index;       /* local encrypt netkey index for mesh message */
}mible_mesh_reset_params_t;

/* relay get/set params */
typedef struct{
    uint16_t dst_addr;
    uint8_t  relay;
    uint8_t  retransmit_count;
    uint8_t  retransmit_interval_steps;
    uint16_t global_netkey_index;       /* local encrypt netkey index for mesh message */
}mible_mesh_relay_params_t;

/* generic message params, like on_off/lightness/... */
typedef struct{
    mible_mesh_address_t dst_addr;
    uint16_t global_netkey_index;
    uint16_t global_appkey_index;
    uint32_t opcode;
    uint16_t element_index;
    uint8_t  data_len;
    uint8_t* data;      /* mesh message raw data */
}mible_mesh_generic_params_t;

/* provision device info */
typedef struct {
    uint8_t address[MIBLE_ADDR_LEN];
    uint8_t address_type;
    uint8_t device_type;
    uint8_t pbtranstype;    /* provision type */
}mible_mesh_remote_device_info_t;

/* unprovision beacon */
typedef struct{
    uint8_t  device_uuid[MIBLE_MESH_DEV_UUID_LEN];
    uint16_t oob_info;
    uint8_t  uri_hash[MIBLE_MESH_URI_HASH_LEN];
    uint8_t  mac[MIBLE_ADDR_LEN];
}mible_mesh_unprov_beacon_t;

/* mesh message meta data */
typedef struct {
    uint16_t src_addr;
    uint16_t dst_addr;
    uint16_t appkey_index;
    uint16_t netkey_index;
    uint8_t  rssi;
    uint8_t  ttl;
} mible_mesh_message_rx_meta_t;

/* mesh status data report*/
typedef struct {
    union{
        uint32_t opcode;
        mible_mesh_opcode_t op;
    };
    mible_mesh_message_rx_meta_t meta_data;
    uint16_t buf_len;       /* mesh raw data len*/
    uint8_t* buf;           /* mesh raw data, see mesh profile 4.3 */
} mible_mesh_access_message_rx_t;

/* ADD contains add and update operation.
 * Firstly, you should execute get-operation, 
 * and then, according to get result, choose you really add or update method.
 * if get-operation return null, you can call add method, otherwise, call update method. */
typedef enum {
    MIBLE_MESH_OP_ADD=0,
    MIBLE_MESH_OP_DELETE,
}mible_mesh_op_t;

typedef struct {
    uint16_t cid;
    uint16_t pdid;
    uint16_t reserved;
    uint8_t  did[4];
    uint8_t  mac[MIBLE_ADDR_LEN];
}mible_mesh_uuid_t;

typedef struct {
    uint16_t unicast_address;
    uint16_t elements_num;
    uint8_t device_key[MIBLE_MESH_KEY_LEN];
    uint8_t uuid[MIBLE_MESH_DEV_UUID_LEN];
    uint16_t netkey_index;
}mible_mesh_node_info_t;

typedef uint32_t mible_mesh_event_t;
typedef int (*mible_mesh_event_cb_t)(mible_mesh_event_t event,void *data);

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
 *@brief    Config Composition Data Get, mesh profile 4.3.2.4, Report 4.3.2.5 Config Composition Data Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] unicast_address: node address
 *@param    [in] netkey_index: key index for node
 *@param    [in] page: page number of the composition data
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_get_composition_data(uint16_t unicast_address, uint16_t global_netkey_index, uint8_t page);

/**
 *@brief    appkey information for node, mesh profile 4.3.2.37-39, Report 4.3.2.40 Config AppKey Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : add/update/delete ...
 *@param    [in] param : appkey parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_appkey(uint32_t opcode, mible_mesh_appkey_params_t *param);

/**
 *@brief    bind appkey information for node, mesh profile 4.3.2.46-47, Report 4.3.2.48 Config Model App Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : bind/unbind
 *@param    [in] param : bind parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_bind_appkey(uint32_t opcode, mible_mesh_model_app_params_t *param);

/**
 *@brief    set publication information for node, mesh profile 4.3.2.15-17,
 *          Report 4.3.2.18 Config Model Publication Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : add/delete/overwrite ...
 *@param    [in] param : publish parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_publication(uint32_t opcode, mible_mesh_publication_params_t * param);

/**
 *@brief    set subscription information for node, mesh profile 4.3.2.19-25.
 *          Report 4.3.2.26 Config Model Subscription Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : add/delete/overwrite ...
 *@param    [in] param : subscription parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_subscription(uint32_t opcode, mible_mesh_subscription_params_t * param);

/**
 *@brief    reset node, 4.3.2.53 Config Node Reset, Report 4.3.2.54 Config Node Reset Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : reset
 *@param    [in] param : reset parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_reset(uint32_t opcode, mible_mesh_reset_params_t *param);

/**
 *@brief    set relay params node, mesh profile 4.3.2.12, Report 4.3.2.14 Config Relay Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : relay
 *@param    [in] param : relay parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_relay_param(uint32_t opcode, mible_mesh_relay_params_t *param);

/**
 *@brief    generic message, Mesh model 3.2, 4.2, 5.2, 6.3, or 7 Summary.
 *          report event: MIBLE_MESH_EVENT_GENERIC_OPTION_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] param : control parameters corresponding to node
 *          according to opcode, generate a mesh message; extral params: ack_opcode, tid, get_or_set.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_generic_control(mible_mesh_generic_params_t * param);

/**
 *@brief    mesh stack init, register event callback
 *          init mesh ble hardware, memory; init mesh thread, prepare for creating mesh network, .
 *          report event: MIBLE_MESH_EVENT_STACK_INIT_DONE, data: NULL.
 *@param    [in] mible_mesh_event_cb : event callback
 *@param    [in] info : init parameters corresponding to gateway
 *@return   0: success, negetive value: failure
 */
int mible_mesh_init_stack(mible_mesh_event_cb_t mible_mesh_event_cb, mible_mesh_gateway_info_t *info);

/**
 *@brief    init mesh provisioner, after this step, mesh network is avilable.
 *          load iv/seq_num, init model, load netkey, bind appkey, sub group address 0xFEFF,
 *          report event: MIBLE_MESH_EVENT_GATEWAY_INIT_DONE, data: NULL.
 *@param    [in] info : init parameters corresponding to gateway
 *@return   0: success, negetive value: failure
 */
int mible_mesh_load_config(mible_mesh_gateway_info_t *info);

/**
 *@brief    set local provisioner network transmit params.
 *@param    [in] count : advertise counter for every adv packet, adv transmit times
 *@param    [in] interval_steps : adv interval = interval_steps*0.625ms
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_network_transmit_param(uint8_t count, uint8_t interval_steps);

/**
 *@brief    update iv index, .
 *@param    [in] iv_index : current IV Index
 *@param    [in] flags : contains the Key Refresh Flag and IV Update Flag
 *@return   0: success, negetive value: failure
 */id);


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
 *@param    [in] model_id : key index for netkey
 *@param    [in] appkey_index : key index for appkey
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_model_app(mible_mesh_op_t op, uint8_t element_index, uint32_t model_id, uint16_t appkey_index);

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
 *@param    [in] param: subscription params
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_sub_address(mible_mesh_op_t op, mible_mesh_subscription_params_t *param);

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

